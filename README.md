# MidiSky

Visualizador de escalas, acordes y notas para aprender piano mientras tocas.
Funciona como plugin **VST3** y **AU** (Ableton Live 12 y otros DAW) y como **app sola**, en Mac y Windows.

- Pantalla central con el acorde en grande, su nombre en español, la inversión y el grado dentro de la tonalidad.
- Pentagrama (clave de sol y de fa) con la armadura de la tonalidad.
- Círculo de quintas: las notas que tocas se unen como una constelación.
- Teclado de 88 teclas que se ilumina con rayos de luz; resalta la escala.
- Tonalidad automática o elegida a mano, con 12 escalas y modos.
- Piano realista integrado (se puede apagar) y control de volumen.
- Skins (Noche, Aurora, Atardecer, Nebulosa, Amanecer, Tormenta, Arcoíris y color personalizado), modo día y noche, ventana redimensionable.
- Conectar y desconectar MIDI sin interrumpir a Ableton.

## Descargar

En **Releases → MidiSky · última versión** están `MidiSky-Mac.zip` y `MidiSky-Windows.zip`.
Cada zip trae un `LEEME.txt` con los pasos para instalarlo.

## Compilar

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

JUCE 8 se descarga solo. GitHub Actions compila las versiones de Mac y Windows en cada cambio.

## Créditos y licencias

- Piano: [Salamander Grand Piano](https://archive.org/details/SalamanderGrandPianoV3) de Alexander Holm (CC BY 3.0).
- Notación: fuente Bravura de Steinberg (SIL Open Font License).
- Tipografía: Outfit (SIL Open Font License).
- Hecho con [JUCE](https://juce.com) bajo AGPLv3. Este proyecto también se distribuye bajo AGPLv3 (ver `LICENSE`).
