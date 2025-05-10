from flask import Flask, render_template, request
from flask_socketio import SocketIO, emit

app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*")

# Rota principal que serve a página HTML
@app.route('/')
def index():
    return render_template('index.html')  

@app.route('/CLICK', methods=['GET', 'POST'])
def click():
    print("Comando: Click")
    socketio.emit('command', {'action': 'click'})  # Envia comando para ON
    return 'Click command sent', 200

@app.route('/SOLTO', methods=['GET', 'POST'])
def solto():
    print("Comando: solto")
    socketio.emit('command', {'action': 'solto'})  # Envia comando para OFF
    return 'solto command sent', 200

@app.route('/TEMP')
def temperature():
    value = request.args.get('value', type=float)
    print(f"Recebida temperatura: {value:.2f} °C")
    socketio.emit('temperature', {'value': value})
    return f"Temperatura {value:.2f} enviada", 200

@app.route('/direcao')
def direcao():
    valor = request.args.get('valor')
    print(f"Direção recebida: {valor}")
    socketio.emit('direcao', {'valor': valor})
    return f"Direção {valor} recebida", 200

if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=5000)

