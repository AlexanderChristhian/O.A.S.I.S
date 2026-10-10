#pragma once

#include <Arduino.h>

const char DASHBOARD_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
  <meta name="theme-color" content="#224cac">
  <title>O.A.S.I.S. Local Dashboard</title>
  <style>
    :root {
      color-scheme: dark;
      --ink: #effbf6;
      --muted: #90aaa2;
      --deep: #071c1b;
      --panel: rgba(14, 45, 42, .78);
      --line: rgba(189, 241, 221, .16);
      --aqua: #72f2c3;
      --lime: #c9f36a;
      --amber: #ffbd66;
      --danger: #ff776d;
      --quality: var(--danger);
      --quality-soft: rgba(255, 119, 109, .13);
      font-family: Inter, ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
    }
    * { box-sizing: border-box; }
    html { background: var(--deep); }
    body {
      margin: 0;
      min-width: 320px;
      min-height: 100vh;
      color: var(--ink);
      background:
        radial-gradient(circle at 88% 6%, rgba(114,242,195,.15), transparent 28rem),
        radial-gradient(circle at 5% 70%, rgba(50,129,119,.17), transparent 32rem),
        linear-gradient(145deg, #061817 0%, #0a2422 58%, #071c1b 100%);
    }
    body::before {
      content: "";
      position: fixed;
      inset: 0;
      pointer-events: none;
      opacity: .32;
      background-image: linear-gradient(rgba(255,255,255,.025) 1px, transparent 1px), linear-gradient(90deg, rgba(255,255,255,.025) 1px, transparent 1px);
      background-size: 42px 42px;
      mask-image: linear-gradient(to bottom, #000, transparent 78%);
    }
    .shell { width: min(1180px, calc(100% - 40px)); margin: 0 auto; padding: 24px 0 34px; position: relative; }
    .topbar { display: flex; align-items: center; justify-content: space-between; gap: 18px; margin-bottom: 28px; }
    .brand { display: flex; align-items: center; gap: 13px; letter-spacing: .16em; font-size: 13px; font-weight: 700; }
    .mark { width: 40px; height: 40px; position: relative; border: 1px solid rgba(114,242,195,.38); transform: rotate(45deg); border-radius: 50% 50% 50% 12%; }
    .mark::after { content: ""; position: absolute; width: 12px; height: 12px; border: 2px solid var(--aqua); border-top-color: transparent; border-radius: 50%; inset: 12px; }
    .brand-copy small { display: block; margin-top: 4px; color: var(--muted); font-size: 9px; letter-spacing: .2em; font-weight: 600; }
    .connection { display: flex; align-items: center; gap: 9px; color: var(--muted); font-size: 12px; }
    .connection-dot { width: 8px; height: 8px; border-radius: 50%; background: var(--danger); box-shadow: 0 0 0 5px rgba(255,119,109,.1); }
    .connection.online .connection-dot { background: var(--aqua); box-shadow: 0 0 0 5px rgba(114,242,195,.1); }

    .hero { display: grid; grid-template-columns: minmax(0,1.25fr) minmax(310px,.75fr); border-top: 1px solid var(--line); border-bottom: 1px solid var(--line); }
    .hero-main { padding: 54px 42px 50px 0; border-right: 1px solid var(--line); }
    .eyebrow { margin: 0 0 17px; color: var(--quality); text-transform: uppercase; letter-spacing: .2em; font-size: 11px; font-weight: 800; }
    .headline { max-width: 760px; margin: 0; font-size: clamp(42px, 7vw, 88px); line-height: .9; letter-spacing: -.055em; font-weight: 650; text-wrap: balance; }
    .headline em { color: var(--quality); font-style: normal; }
    .summary { max-width: 590px; margin: 24px 0 0; color: var(--muted); font-size: 15px; line-height: 1.65; }
    .quality-line { display: flex; flex-wrap: wrap; align-items: center; gap: 12px; margin-top: 30px; }
    .quality-badge { padding: 9px 13px; border-left: 3px solid var(--quality); background: var(--quality-soft); color: var(--quality); font-size: 12px; letter-spacing: .12em; font-weight: 800; }
    .mode-readout { color: var(--muted); font-size: 12px; letter-spacing: .08em; }
    .mode-readout strong { color: var(--ink); }

    .sensor-stack { display: grid; grid-template-columns: 1fr 1fr; align-content: center; gap: 1px; padding: 34px 0 34px 34px; }
    .sensor { min-height: 214px; padding: 26px 22px; background: rgba(4,22,21,.42); position: relative; overflow: hidden; }
    .sensor::after { content: ""; position: absolute; width: 120px; height: 120px; right: -46px; bottom: -50px; border: 1px solid var(--line); border-radius: 50%; box-shadow: 0 0 0 24px rgba(114,242,195,.025), 0 0 0 48px rgba(114,242,195,.018); }
    .sensor-label { color: var(--muted); text-transform: uppercase; letter-spacing: .16em; font-size: 10px; }
    .sensor-value { margin-top: 22px; font-size: clamp(32px, 4vw, 54px); line-height: 1; letter-spacing: -.06em; font-variant-numeric: tabular-nums; }
    .sensor-value small { margin-left: 4px; color: var(--muted); font-size: 12px; letter-spacing: .02em; }
    .meter { height: 3px; margin-top: 26px; background: rgba(255,255,255,.08); position: relative; }
    .meter span { display: block; width: var(--pct, 0%); height: 100%; max-width: 100%; background: var(--quality); transition: width .35s ease, background .35s ease; }
    .sensor-note { margin-top: 11px; color: var(--muted); font-size: 10px; }
    .battery-sensor { grid-column: 1 / -1; min-height: 150px; display: grid; grid-template-columns: minmax(0, 1fr) minmax(115px, .42fr); align-items: center; gap: 24px; }
    .battery-sensor .sensor-value { margin-top: 15px; }
    .battery-sensor .meter { margin-top: 19px; }
    .battery-sensor .meter span { background: var(--aqua); }
    .battery-voltage { margin-top: 14px; font-size: 27px; line-height: 1; font-weight: 700; font-variant-numeric: tabular-nums; }
    .battery-voltage small { margin-left: 4px; color: var(--muted); font-size: 12px; font-weight: 400; }

    .workspace { display: grid; grid-template-columns: minmax(310px,.72fr) minmax(0,1.28fr); gap: 22px; margin-top: 22px; }
    .section { background: var(--panel); border: 1px solid var(--line); backdrop-filter: blur(16px); }
    .section-head { display: flex; align-items: baseline; justify-content: space-between; gap: 12px; padding: 20px 22px; border-bottom: 1px solid var(--line); }
    .section-title { margin: 0; font-size: 12px; letter-spacing: .16em; text-transform: uppercase; }
    .section-kicker { color: var(--muted); font-size: 10px; }
    .current-route { margin: 18px; padding: 24px 20px; border-left: 5px solid var(--aqua); background: rgba(114,242,195,.11); }
    .route-kind { display: block; margin-bottom: 12px; color: var(--muted); font-size: 10px; letter-spacing: .12em; text-transform: uppercase; }
    .route-name { display: block; font-size: 32px; font-weight: 750; letter-spacing: .06em; }
    .control-note { margin: 0; padding: 0 20px 20px; color: var(--muted); font-size: 11px; line-height: 1.55; }

    .outputs { display: grid; grid-template-columns: repeat(4, 1fr); gap: 1px; background: var(--line); }
    .output { min-height: 172px; padding: 21px 18px; background: rgba(7,28,27,.94); }
    .output-state { display: flex; justify-content: space-between; align-items: center; gap: 8px; }
    .output-dot { width: 10px; height: 10px; border: 1px solid #607872; border-radius: 50%; }
    .output.on .output-dot { border-color: var(--aqua); background: var(--aqua); box-shadow: 0 0 18px rgba(114,242,195,.62); }
    .output-word { color: var(--muted); font: 10px ui-monospace, monospace; letter-spacing: .1em; }
    .output.on .output-word { color: var(--aqua); }
    .output-name { margin-top: 54px; font-size: 14px; line-height: 1.25; }
    .output-pin { display: block; margin-top: 7px; color: var(--muted); font: 10px ui-monospace, monospace; }

    .foot { display: flex; flex-wrap: wrap; justify-content: space-between; gap: 10px 22px; margin-top: 18px; color: var(--muted); font-size: 10px; letter-spacing: .08em; text-transform: uppercase; }
    .foot strong { color: var(--ink); font-weight: 650; }
    body[data-quality="DRINKABLE"] { --quality: var(--aqua); --quality-soft: rgba(114,242,195,.12); }
    body[data-quality="MCK ONLY"] { --quality: var(--amber); --quality-soft: rgba(255,189,102,.12); }
    body[data-quality="UNSAFE"] { --quality: var(--danger); --quality-soft: rgba(255,119,109,.13); }

    @media (max-width: 850px) {
      .shell { width: min(100% - 28px, 720px); padding-top: 18px; }
      .hero { grid-template-columns: 1fr; }
      .hero-main { padding: 42px 0 38px; border-right: 0; }
      .sensor-stack { padding: 0 0 22px; }
      .workspace { grid-template-columns: 1fr; }
    }
    @media (max-width: 560px) {
      .shell { width: min(100% - 20px, 520px); }
      .brand-copy small { display: none; }
      .connection span:last-child { display: none; }
      .topbar { margin-bottom: 14px; }
      .hero-main { padding: 35px 2px 32px; }
      .headline { font-size: clamp(40px, 15vw, 62px); }
      .summary { font-size: 13px; }
      .sensor-stack { grid-template-columns: 1fr 1fr; gap: 8px; }
      .sensor { min-height: 160px; padding: 19px 16px; }
      .sensor-value { margin-top: 18px; font-size: 34px; }
      .meter { margin-top: 19px; }
      .battery-sensor { min-height: 140px; gap: 12px; }
      .battery-sensor .sensor-value { margin-top: 14px; }
      .battery-voltage { font-size: 23px; }
      .current-route { margin: 12px; padding: 20px 16px; }
      .route-name { font-size: 28px; }
      .outputs { grid-template-columns: 1fr 1fr; }
      .output { min-height: 130px; }
      .output-name { margin-top: 30px; }
      .foot { padding-bottom: env(safe-area-inset-bottom); }
    }
    @media (prefers-reduced-motion: reduce) { * { scroll-behavior: auto !important; transition: none !important; } }

    /* Presentation-matched O.A.S.I.S. visual system */
    :root {
      color-scheme: light;
      --ink: #17191f;
      --muted: #646b80;
      --deep: #f4f6fb;
      --panel: #ffffff;
      --line: #d6dced;
      --aqua: #224cac;
      --lime: #fdc70d;
      --amber: #d49400;
      --danger: #c73546;
      --quality: var(--danger);
      --quality-soft: rgba(199, 53, 70, .09);
      font-family: "Poppins", "Montserrat", "Avenir Next", Arial, sans-serif;
    }
    html { background: #f4f6fb; }
    body {
      color: var(--ink);
      background:
        radial-gradient(circle at 82% 18%, rgba(34,76,172,.08), transparent 24rem),
        linear-gradient(120deg, #fff 0%, #f7f8fc 62%, #eef2fb 100%);
    }
    body::before {
      inset: 0 auto 0 0;
      width: 14px;
      opacity: 1;
      background: linear-gradient(to bottom, #fdc70d 0 18%, #224cac 18% 63%, #262e6e 63% 100%);
      background-size: auto;
      mask-image: none;
    }
    body::after {
      content: "";
      position: fixed;
      z-index: -1;
      top: 0;
      right: 0;
      width: min(31vw, 390px);
      height: 180px;
      background: linear-gradient(135deg, transparent 0 38%, #fdc70d 38% 50%, #224cac 50% 70%, #262e6e 70% 100%);
      clip-path: polygon(42% 0, 100% 0, 100% 100%);
      opacity: .96;
    }
    .shell { width: min(1200px, calc(100% - 58px)); padding-top: 22px; }
    .topbar { margin-bottom: 24px; padding: 0 0 18px; border-bottom: 4px solid #17191f; }
    .brand { color: #224cac; letter-spacing: .09em; font-size: 16px; font-weight: 800; }
    .brand-copy small { color: #596583; letter-spacing: .07em; font-size: 9px; font-weight: 600; }
    .mark {
      width: 43px;
      height: 43px;
      transform: none;
      border: 0;
      border-radius: 0;
      background: linear-gradient(135deg, #fdc70d 0 42%, #224cac 42% 72%, #262e6e 72%);
      clip-path: polygon(0 0, 100% 0, 72% 100%, 0 100%);
    }
    .mark::after { width: 11px; height: 11px; inset: 13px; border-color: #fff; border-top-color: transparent; }
    .connection { color: #4f5871; font-weight: 600; }
    .connection-dot { background: var(--danger); box-shadow: 0 0 0 5px rgba(199,53,70,.1); }
    .connection.online .connection-dot { background: #224cac; box-shadow: 0 0 0 5px rgba(34,76,172,.12); }

    .hero {
      position: relative;
      grid-template-columns: minmax(0,1.22fr) minmax(310px,.78fr);
      overflow: hidden;
      border: 1px solid #d8deed;
      border-left: 12px solid #224cac;
      background: rgba(255,255,255,.94);
      box-shadow: 0 18px 45px rgba(30,51,103,.09);
    }
    .hero::after {
      content: "";
      position: absolute;
      right: -72px;
      bottom: -92px;
      width: 210px;
      height: 210px;
      transform: rotate(18deg);
      background: linear-gradient(135deg, transparent 0 34%, rgba(253,199,13,.22) 34% 58%, rgba(34,76,172,.13) 58% 76%, transparent 76%);
      pointer-events: none;
    }
    .hero-main { padding: 51px 42px 47px 38px; border-right-color: #d8deed; }
    .eyebrow { color: #224cac; letter-spacing: .14em; font-weight: 700; }
    .headline { color: #17191f; font-weight: 800; letter-spacing: -.045em; text-shadow: 0 3px 0 rgba(34,76,172,.07); }
    .headline em { color: var(--quality); }
    .summary { color: #555d73; }
    .quality-badge { border-left-width: 5px; background: var(--quality-soft); color: var(--quality); }
    .mode-readout { color: #687089; }
    .mode-readout strong { color: #262e6e; }

    .sensor-stack { background: #edf1fa; }
    .sensor { background: rgba(255,255,255,.91); border-top: 8px solid #fdc70d; }
    .sensor::after { border-color: rgba(34,76,172,.16); box-shadow: 0 0 0 24px rgba(34,76,172,.035), 0 0 0 48px rgba(253,199,13,.045); }
    .sensor-label { color: #224cac; font-weight: 700; }
    .sensor-value { color: #262e6e; font-weight: 700; }
    .sensor-value small, .sensor-note { color: #646b80; }
    .battery-voltage { color: #262e6e; }
    .meter { background: #dce2f0; }

    .workspace { gap: 24px; }
    .section { border: 1px solid #cfd7e9; background: #fff; box-shadow: 0 12px 30px rgba(31,54,110,.07); backdrop-filter: none; }
    .section-head { padding: 16px 20px; border-bottom: 0; background: #fdc70d; color: #17191f; }
    .section-title { color: #17191f; font-weight: 800; letter-spacing: .07em; }
    .section-kicker { color: #3a3e49; font-weight: 600; }
    .current-route { border-left-color: #224cac; background: #eef2fb; color: #262e6e; }
    .current-route[data-mode="OFF"] { border-left-color: #d49400; background: #fff8df; }
    .route-kind { color: #646b80; font-weight: 700; }
    .control-note { color: #5e667b; }

    .outputs { gap: 6px; padding: 6px; background: #fff; }
    .output { background: #262e6e; color: #fff; border-bottom: 7px solid #224cac; }
    .output.on { background: #224cac; border-bottom-color: #fdc70d; }
    .output-dot { border-color: #9ea9cd; }
    .output.on .output-dot { border-color: #fdc70d; background: #fdc70d; box-shadow: 0 0 0 5px rgba(253,199,13,.17); }
    .output-word { color: #c5cbe0; }
    .output.on .output-word { color: #fff1a4; }
    .output-pin { color: #c5cbe0; }
    .foot { color: #5c6478; }
    .foot strong { color: #224cac; }

    body[data-quality="DRINKABLE"] { --quality: #224cac; --quality-soft: rgba(34,76,172,.09); }
    body[data-quality="MCK ONLY"] { --quality: #c58a00; --quality-soft: rgba(253,199,13,.16); }
    body[data-quality="UNSAFE"] { --quality: #c73546; --quality-soft: rgba(199,53,70,.09); }

    @media (max-width: 850px) {
      .shell { width: min(100% - 38px, 720px); }
      .hero { grid-template-columns: 1fr; border-left-width: 9px; }
      .hero-main { padding-left: 28px; }
      .sensor-stack { background: #edf1fa; padding: 10px; }
    }
    @media (max-width: 560px) {
      body::before { width: 8px; }
      body::after { width: 190px; height: 105px; opacity: .82; }
      .shell { width: min(100% - 28px, 520px); padding-top: 14px; }
      .topbar { border-bottom-width: 3px; }
      .brand { font-size: 13px; }
      .hero { border-left-width: 7px; }
      .hero-main { padding: 34px 16px 31px 20px; }
      .headline { font-size: clamp(38px, 14vw, 58px); }
      .sensor { border-top-width: 6px; }
      .section-head { padding: 14px 16px; }
      .outputs { gap: 4px; padding: 4px; }
    }
  </style>
</head>
<body data-quality="UNSAFE">
  <div class="shell">
    <header class="topbar">
      <div class="brand">
        <span class="mark" aria-hidden="true"></span>
        <span class="brand-copy">O.A.S.I.S.<small>OPTIMIZED AUTONOMOUS SYSTEM FOR IMPURE SOURCES</small></span>
      </div>
      <div class="connection" id="connection"><span class="connection-dot"></span><span id="connectionText">Connecting to controller</span></div>
    </header>

    <main>
      <section class="hero" aria-labelledby="headline">
        <div class="hero-main">
          <p class="eyebrow">Live quality gate / local network</p>
          <h1 class="headline" id="headline">OUTPUT <em>INTERLOCKED</em></h1>
          <p class="summary" id="summary">Waiting for the first sensor reading. Unsafe readings or a mode mismatch keep every water outlet closed.</p>
          <div class="quality-line">
            <span class="quality-badge" id="quality">UNSAFE</span>
            <span class="mode-readout">CURRENT ROUTE <strong id="modeReadout">--</strong></span>
          </div>
        </div>
        <div class="sensor-stack" aria-label="Sensor readings">
          <article class="sensor">
            <div class="sensor-label">Acidity balance</div>
            <div class="sensor-value"><span id="phValue">--</span><small>pH</small></div>
            <div class="meter" aria-hidden="true"><span id="phMeter"></span></div>
            <div class="sensor-note">Accepted band 6.5—8.5</div>
          </article>
          <article class="sensor">
            <div class="sensor-label">Optical clarity</div>
            <div class="sensor-value"><span id="ntuValue">--</span><small>NTU</small></div>
            <div class="meter" aria-hidden="true"><span id="ntuMeter"></span></div>
            <div class="sensor-note">Drink ≤1.0 · MCK &lt;3.0</div>
          </article>
          <article class="sensor battery-sensor" aria-label="12 volt VRLA battery status">
            <div>
              <div class="sensor-label">12 V VRLA battery</div>
              <div class="sensor-value"><span id="batteryPercent">--</span><small>%</small></div>
              <div class="meter" aria-hidden="true"><span id="batteryMeter"></span></div>
              <div class="sensor-note">Charge estimate from voltage</div>
            </div>
            <div>
              <div class="sensor-label">Measured voltage</div>
              <div class="battery-voltage"><span id="batteryVoltage">--</span><small>V</small></div>
            </div>
          </article>
        </div>
      </section>

      <div class="workspace">
        <section class="section" aria-labelledby="modeTitle">
          <div class="section-head"><h2 class="section-title" id="modeTitle">Current route</h2><span class="section-kicker">Sensor interlocked</span></div>
          <div class="current-route" id="currentRoute" role="status" aria-live="polite" aria-atomic="true">
            <span class="route-kind" id="routeKind">Waiting for controller</span>
            <strong class="route-name" id="routeName">--</strong>
          </div>
          <p class="control-note">Shows the mode selected on the controller. Use the physical selector to change it. Water outputs still depend on pH and turbidity limits.</p>
        </section>

        <section class="section" aria-labelledby="outputTitle">
          <div class="section-head"><h2 class="section-title" id="outputTitle">Hydraulic outputs</h2><span class="section-kicker" id="routeLabel">Route locked</span></div>
          <div class="outputs">
            <article class="output" id="outHose"><div class="output-state"><span class="output-dot"></span><span class="output-word">OFF</span></div><div class="output-name">Hose pump<span class="output-pin">GPIO 18</span></div></article>
            <article class="output" id="outMck"><div class="output-state"><span class="output-dot"></span><span class="output-word">CLOSED</span></div><div class="output-name">MCK valve<span class="output-pin">GPIO 19</span></div></article>
            <article class="output" id="outRo"><div class="output-state"><span class="output-dot"></span><span class="output-word">OFF</span></div><div class="output-name">RO pump<span class="output-pin">GPIO 23</span></div></article>
            <article class="output" id="outDrink"><div class="output-state"><span class="output-dot"></span><span class="output-word">CLOSED</span></div><div class="output-name">Drink valve<span class="output-pin">GPIO 5</span></div></article>
          </div>
        </section>
      </div>
    </main>

    <footer class="foot"><span>Local endpoint <strong id="endpoint">192.168.4.1</strong></span><span>Last sample <strong id="lastUpdate">—</strong></span><span>Firmware state is authoritative</span></footer>
  </div>

  <script>
    const $ = (id) => document.getElementById(id);
    const outputMap = [
      ['outHose', 'hosePump', 'ON', 'OFF'],
      ['outMck', 'mckValve', 'OPEN', 'CLOSED'],
      ['outRo', 'roPump', 'ON', 'OFF'],
      ['outDrink', 'drinkValve', 'OPEN', 'CLOSED']
    ];
    const routeKinds = {MCK: 'Sanitation', OFF: 'Safe stop', DRINK: 'RO line'};

    function render(data) {
      document.body.dataset.quality = data.waterClass;
      $('quality').textContent = data.waterClass;
      $('modeReadout').textContent = data.mode;
      $('phValue').textContent = Number(data.ph).toFixed(2);
      $('ntuValue').textContent = Number(data.turbidity).toFixed(2);
      $('phMeter').style.setProperty('--pct', Math.min(100, data.ph / 14 * 100) + '%');
      $('ntuMeter').style.setProperty('--pct', Math.min(100, data.turbidity / 50 * 100) + '%');
      const batteryPercent = Number(data.batteryPercent);
      const batteryVoltage = Number(data.batteryVoltage);
      $('batteryPercent').textContent = Number.isFinite(batteryPercent) ? Math.round(batteryPercent) : '--';
      $('batteryVoltage').textContent = Number.isFinite(batteryVoltage) ? batteryVoltage.toFixed(2) : '--';
      $('batteryMeter').style.setProperty('--pct', Number.isFinite(batteryPercent) ? Math.max(0, Math.min(100, batteryPercent)) + '%' : '0%');
      $('currentRoute').dataset.mode = data.mode;
      $('routeName').textContent = data.mode;
      $('routeKind').textContent = routeKinds[data.mode] || 'Unknown route';

      const copy = data.waterClass === 'DRINKABLE'
        ? ['CLEAR FOR', 'DRINK ROUTE', 'Current pH and turbidity satisfy the simulation drinking-route limits. The DRINK output still requires DRINK mode.']
        : data.waterClass === 'MCK ONLY'
          ? ['SANITATION', 'ROUTE ONLY', 'Water is inside the MCK simulation band but does not qualify for the drinking route.']
          : ['OUTPUT', 'INTERLOCKED', 'The current sensor reading is outside an accepted range. Water delivery remains blocked.'];
      $('headline').innerHTML = copy[0] + ' <em>' + copy[1] + '</em>';
      $('summary').textContent = copy[2];

      outputMap.forEach(([id, key, onWord, offWord]) => {
        const node = $(id);
        const on = Boolean(data.outputs[key]);
        node.classList.toggle('on', on);
        node.querySelector('.output-word').textContent = on ? onWord : offWord;
      });
      const active = outputMap.filter(([, key]) => data.outputs[key]).length;
      $('routeLabel').textContent = active ? active + ' output' + (active > 1 ? 's' : '') + ' active' : 'Route locked';
      $('endpoint').textContent = location.host || data.apIp;
      $('lastUpdate').textContent = new Date().toLocaleTimeString([], {hour:'2-digit', minute:'2-digit', second:'2-digit'});
      $('connection').classList.add('online');
      $('connectionText').textContent = 'Controller online';
    }

    async function refresh() {
      try {
        const response = await fetch('/api/status', {cache: 'no-store'});
        if (!response.ok) throw new Error('status ' + response.status);
        render(await response.json());
      } catch (error) {
        $('connection').classList.remove('online');
        $('connectionText').textContent = 'Controller unavailable';
      }
    }

    refresh();
    setInterval(refresh, 750);
  </script>
</body>
</html>
)HTML";
