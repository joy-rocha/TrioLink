'use strict';
const fs = require('fs'), path = require('path'), http = require('http');
const mqtt = require('mqtt');
const { WebSocketServer } = require('ws');

const cfg = JSON.parse(fs.readFileSync(path.join(__dirname, 'config.json'), 'utf8'));
const TOPIC = 'fusion/bmp_mpu', STATUS = ['status/rasp1', 'status/rasp2', 'status/rasp3'];
const DATA_DIR = path.join(__dirname, 'data'), FILE = path.join(DATA_DIR, 'historico.txt');
const HEAD = 'timestamp_utc\ttemperatura_C\tpressao_Pa\taltitude_m\tbmp_stale\taccel_x\taccel_y\taccel_z\tgyro_x\tgyro_y\tgyro_z';
const log = (...a) => console.log(new Date().toLocaleTimeString('pt-BR'), ...a);

/* ---------- histórico em .txt ---------- */
fs.mkdirSync(DATA_DIR, { recursive: true });
const resetFile = () => fs.writeFileSync(FILE, HEAD + '\n');
if (!fs.existsSync(FILE) || fs.statSync(FILE).size === 0) resetFile();

const state = { broker: { connected: false, detail: 'conectando…' }, status: {}, recent: [], lines: [], count: 0, lastAt: 0 };
{
  const rows = fs.readFileSync(FILE, 'utf8').split('\n').filter(Boolean).slice(1);
  state.count = rows.length; state.lines = rows.slice(-40);
}
const toLine = m => {
  const b = m.bmp || {}, u = m.mpu || {}, a = u.accel || {}, g = u.gyro || {};
  const ts = Number.isFinite(m.timestamp) ? m.timestamp : Date.now();
  return [new Date(ts).toISOString(), b.temperature, b.pressure, b.altitude, !!b.stale, a.x, a.y, a.z, g.x, g.y, g.z]
    .map(v => (v === undefined || v === null ? '' : v)).join('\t');
};

/* ---------- WebSocket para o painel ---------- */
const server = http.createServer((req, res) => {
  const url = req.url.split('?')[0];
  if (req.method === 'GET' && (url === '/' || url === '/index.html')) {
    res.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8' });
    return res.end(fs.readFileSync(path.join(__dirname, 'public', 'index.html')));
  }
  if (req.method === 'GET' && url === '/historico.txt') {
    const name = 'historico_fusion_' + new Date().toISOString().slice(0, 19).replace(/[:T]/g, '-') + '.txt';
    res.writeHead(200, { 'Content-Type': 'text/plain; charset=utf-8', 'Content-Disposition': `attachment; filename="${name}"` });
    return fs.createReadStream(FILE).pipe(res);
  }
  if (req.method === 'POST' && url === '/api/clear') {
    resetFile(); state.count = 0; state.lines = []; state.recent = [];
    res.writeHead(204); return res.end();
  }
  res.writeHead(404); res.end('Não encontrado');
});
const wss = new WebSocketServer({ server, path: '/ws' });
const send = (ws, o) => { if (ws.readyState === 1) ws.send(JSON.stringify(o)); };
const broadcast = o => wss.clients.forEach(ws => send(ws, o));
wss.on('connection', ws => send(ws, {
  type: 'snapshot', broker: state.broker, status: state.status, recent: state.recent, lines: state.lines,
  count: state.count, age: state.lastAt ? Date.now() - state.lastAt : null
}));

/* ---------- MQTT (TLS na 8883, CA + usuário/senha) ---------- */
const b = cfg.broker;
const opts = {
  protocol: 'mqtts', host: b.host, port: b.port, username: b.username, password: b.password,
  ca: fs.readFileSync(path.resolve(__dirname, b.caFile)), rejectUnauthorized: true,
  clientId: 'painel_web_' + Math.random().toString(16).slice(2, 8),
  keepalive: 60, reconnectPeriod: 3000, connectTimeout: 10000, clean: true
};
// A CA continua sendo validada; só ignora a checagem de nome/IP do certificado do servidor.
if (b.verifyHostname === false) opts.checkServerIdentity = () => undefined;

const setBroker = (connected, detail) => {
  state.broker = { connected, detail }; broadcast({ type: 'broker', ...state.broker }); log('Broker:', connected ? 'conectado' : 'offline', detail);
};
const client = mqtt.connect(opts);
client.on('connect', () => {
  client.subscribe([TOPIC, ...STATUS], { qos: 0 });
  setBroker(true, `${b.host}:${b.port} (TLS)`);
});
client.on('reconnect', () => setBroker(false, 'reconectando…'));
client.on('close', () => { if (state.broker.connected) setBroker(false, 'conexão perdida'); });
client.on('error', e => setBroker(false, String(e.message || e)));

client.on('message', (topic, payload) => {
  const raw = payload.toString();
  if (topic === TOPIC) {
    let m; try { m = JSON.parse(raw); } catch { return log('JSON inválido em', TOPIC); }
    if (!m || typeof m !== 'object') return;
    const line = toLine(m);
    try { fs.appendFileSync(FILE, line + '\n'); } catch (e) { log('Erro ao gravar:', e.message); }
    state.count++; state.lastAt = Date.now();
    state.lines.push(line); if (state.lines.length > 40) state.lines.shift();
    state.recent.push(m); if (state.recent.length > 120) state.recent.shift();
    broadcast({ type: 'data', msg: m, line, count: state.count });
    return;
  }
  const i = STATUS.indexOf(topic);
  if (i >= 0) {
    const s = { raw, at: Date.now() }; state.status[i + 1] = s;
    broadcast({ type: 'status', n: i + 1, raw, at: s.at });
  }
});

server.listen(cfg.web.port, cfg.web.host, () => log(`Painel em http://localhost:${cfg.web.port}`));
