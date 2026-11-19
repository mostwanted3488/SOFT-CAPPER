# SOFT CAPPER

Soft clipper **one knob** avec **gain automatique** — par BUYORBEALOSER.
Plus tu montes le knob, plus le nez du perso s'allonge… et plus les casquettes s'empilent sur sa tête.

## Le son

| Contrôle | Ce que ça fait |
|---|---|
| **Knob (0–100 %)** | Drive de 0 à +24 dB dans un soft clipper à genou doux (linéaire sous ≈ −5 dB, arrondi en tanh au-dessus), suréchantillonné 4×, plafond à −0,1 dBFS. |
| **Gain auto** | Ramène le volume perçu (RMS) au niveau de l'entrée : tu entends la densité, pas juste « plus fort ». |
| **LOUD** | Jusqu'à +9 dB au-dessus du niveau d'origine (à 100 %), façon maximizer, sans jamais dépasser −0,1 dBFS. |

Le bypass se fait depuis le DAW.

## Télécharger

Onglet **Actions** du repo → dernier build vert → section *Artifacts* :

- **SOFT-CAPPER-Windows-VST3** → copier `SOFT CAPPER.vst3` dans `C:\Program Files\Common Files\VST3\`
- **SOFT-CAPPER-macOS-VST3-AU** → dézipper, puis
  - `SOFT CAPPER.vst3` → `~/Library/Audio/Plug-Ins/VST3/`
  - `SOFT CAPPER.component` → `~/Library/Audio/Plug-Ins/Components/`
  - Le plugin n'est pas notarisé : si macOS le bloque, lancer dans le Terminal
    `xattr -cr ~/Library/Audio/Plug-Ins/VST3/"SOFT CAPPER.vst3" ~/Library/Audio/Plug-Ins/Components/"SOFT CAPPER.component"`

Dans FL Studio : *Options → Manage plugins → Find more plugins*, puis chercher **SOFT CAPPER**.

## Interface

L'interface est la même scène HTML/SVG que la preview (dossier `ui/`), affichée dans une WebView native
(WebView2 sur Windows, WKWebView sur macOS). Windows 10/11 ont déjà le runtime WebView2.
Polices incluses : Anton, JetBrains Mono (OFL) et Permanent Marker (Apache 2.0) — licences dans `ui/fonts/`.

## Compiler soi-même

```bash
cmake -S . -B build            # JUCE 8 est téléchargé automatiquement
cmake --build build --config Release
```
Sur Windows, il faut le paquet NuGet `Microsoft.Web.WebView2` (voir `.github/workflows/build.yml`).
