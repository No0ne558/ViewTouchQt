# Remote Display Guide

This document explains how to run `vt_client` on remote or external displays,
including Android devices running **XServer XSDL**.

---

## 1. Local X11 (default)

```bash
./vt_client --server 127.0.0.1 --port 12000
```

Qt automatically uses the `xcb` platform plugin when `DISPLAY` is set.

---

## 2. Local Wayland

```bash
./vt_client -platform wayland --server 127.0.0.1 --port 12000
```

Requires a running Wayland compositor (Sway, GNOME Wayland, KDE Wayland, etc.).

---

## 3. Remote X11 display (another Linux machine)

On the **remote machine**, allow connections:

```bash
xhost +<host-ip>
```

On the **host**, launch the client pointing at the remote display:

```bash
DISPLAY=<remote-ip>:0 ./vt_client -platform xcb --server <host-ip> --port 12000
```

---

## 4. XServer XSDL (Android)

[XServer XSDL](https://play.google.com/store/apps/details?id=x.org.server) runs
a full X11 server on an Android device, allowing you to send any X11 application
to the phone/tablet screen.

### Setup

1. Install **XServer XSDL** from the Play Store on the Android device.
2. Launch the app — it will display the IP address and display number
   (usually `:0`).  Note the IP (e.g. `192.168.1.50`).
3. Make sure the Android device and the host are on the **same network**.
4. On the host, launch the client:

```bash
DISPLAY=192.168.1.50:0 ./vt_client -platform xcb --server <host-ip> --port 12000
```

The fullscreen POS UI will appear on the Android device's screen. Touch input
is forwarded back to the app via the X11 protocol.

### Tips

- **Performance:** XServer XSDL works over the network. For best results, use a
  wired connection or a 5 GHz Wi-Fi network.
- **Resolution:** The client auto-scales from its 1920×1080 design resolution to
  whatever resolution XServer XSDL reports. No configuration needed.
- **Multiple terminals:** You can launch multiple `vt_client` instances, each on
  a different Android device, all connecting to the same `vt_server`.

---

## 5. Environment variables

| Variable | Effect |
|---|---|
| `DISPLAY=<ip>:<n>` | Target X11 display (XServer XSDL, remote X11, etc.) |
| `QT_SCALE_FACTOR=<f>` | Override Qt DPI scaling factor (e.g. `1.5`) |
| `QT_QPA_PLATFORM=xcb` | Force X11 backend |
| `QT_QPA_PLATFORM=wayland` | Force Wayland backend |

---

## 6. Architecture diagram

```
┌──────────────────┐      TCP :12000        ┌──────────────────┐
│    vt_server     │◄──────────────────────►│    vt_client     │
│  (host, headless)│                        │ (local or remote │
│                  │                        │   X11 display)   │
└──────────────────┘                        └──────────────────┘
                                                    │
                                               X11 protocol
                                                    │
                                            ┌───────▼────────┐
                                            │  XServer XSDL  │
                                            │   (Android)    │
                                            └────────────────┘
```
