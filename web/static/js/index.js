const socket = io();

const box = document.getElementById('status-box');
const tempDisplay = document.getElementById('temp-box');
const dirDisplay = document.getElementById('direcao');
let isOn = false;

socket.on('connect', () => {
  console.log('Conectado ao servidor via Socket.IO');
});

socket.on('temperature', data => {
  console.log('Temperatura:', data.value);
  tempDisplay.textContent = `Temperatura: ${data.value.toFixed(2)} °C`;
});

socket.on('command', (data) => {
  if (data.action === 'click') {
    if (!isOn) {
      box.style.backgroundColor = 'green';
      box.textContent = 'ON';
      isOn = true;
    }
  } else if (data.action === 'solto') {
    if (isOn) {
      box.style.backgroundColor = 'red';
      box.textContent = 'OFF';
      isOn = false;
    }
  }
});

socket.on('direcao', data => {
  // limpa anterior
  document
    .querySelectorAll('#compass polygon.active')
    .forEach(el => el.classList.remove('active'));

  // destaca o novo
  const dir = data.valor;           
  const poly = document.getElementById(dir);
  if (poly) poly.classList.add('active');
});

// // opcional: clique direto para teste
// document.querySelectorAll('#compass polygon')
//   .forEach(poly => {
//     poly.addEventListener('click', () => {
//       socket.emit('direcao', { valor: poly.id });
//     });
//   });
