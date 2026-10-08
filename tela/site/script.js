/* TrioLink - lê o JSON gerado pelo programa em C (via server.py) e atualiza a interface */
const $ = (id) => document.getElementById(id);
const fmt = (v, d = 0) => (typeof v === "number" ? v.toFixed(d) : "--");

/* ---------- navegação entre telas ---------- */
function show(name) {
    document.querySelectorAll(".screen").forEach((s) => (s.hidden = s.id !== "scr-" + name));
    const first = document.querySelector(`#scr-${name} button`);
    if (first) first.focus({ preventScroll: true });
}

document.addEventListener("click", (e) => {
    const b = e.target.closest("[data-go]");
    if (b) show(b.dataset.go);
});

/* ---------- estado (NORMAL / ANORMAL / OFFLINE) ---------- */
function setEstado(el, s) {
    if (!s || !s.online) { el.textContent = "OFFLINE"; el.className = "bad"; }
    else if (s.estado === "anormal") { el.textContent = "ANORMAL"; el.className = "bad"; }
    else { el.textContent = "NORMAL"; el.className = "ok"; }
}

/* ---------- aplica os dados na tela ---------- */
function render(data) {
    const bmp = data.bmp || {}, mpu = data.mpu || {};

    $("led-bmp").classList.toggle("on", !!bmp.online);
    $("led-mpu").classList.toggle("on", !!mpu.online);

    $("bmp-p").textContent = bmp.online ? fmt(bmp.pressao) + " hPa" : "--";
    $("bmp-t").textContent = bmp.online ? fmt(bmp.temperatura) + "° C" : "--";
    $("bmp-a").textContent = bmp.online ? fmt(bmp.altitude) + " m" : "--";
    setEstado($("bmp-e"), bmp);

    $("mpu-d").textContent = mpu.online ? fmt(mpu.direcao) + " deg" : "--";
    $("mpu-v").textContent = mpu.online ? fmt(mpu.velocidade, 1) + " m/s" : "--";
    setEstado($("mpu-e"), mpu);
}

/* ---------- polling a cada 1 s ---------- */
async function poll() {
    try {
        const r = await fetch("/api/data", { cache: "no-store" });
        if (!r.ok) throw new Error(r.status);
        render(await r.json());
        $("conn").textContent = "";
    } catch (_) {
        render({});
        $("conn").textContent = "Sem conexão com o sistema. Verifique se o programa em C e o server.py estão rodando.";
    }
}
poll();
setInterval(poll, 1000);

/* ---------- desligar ---------- */
$("btn-yes").addEventListener("click", async () => {
    $("btn-yes").disabled = true;
    try { await fetch("/api/shutdown", { method: "POST" }); } catch (_) {}
    document.querySelector("#scr-off .ask").innerHTML = "DESLIGANDO...";
    document.querySelector("#scr-off .yesno").hidden = true;
});
