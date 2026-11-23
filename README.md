# MidiCat 🐱

(Antes se llamaba MidiSky y luego "Midi Cat".)

Visualizador de escalas, acordes y notas para aprender piano mientras tocas.
Funciona como plugin **VST3** y **AU** (Ableton Live 12 y otros DAW) y como **app sola**, en Mac y Windows.

- Pantalla central con el acorde en grande, su nombre en español y el grado dentro de la tonalidad; los grados de la escala abajo (I a VII, se pueden tocar para escucharlos y elegir su octava).
- Pentagrama (clave de sol y de fa) con la armadura de la tonalidad.
- Círculo de quintas: las notas que tocas se unen con líneas animadas (rodean el centro, pasan por debajo, se difuminan o pasan por encima).
- Teclado de 2, 3 o 4 octavas que sigue lo que tocas, con 4 formas de teclas (Pastilla, Botones, Plana y Clásica) y color a tu gusto.
- Siete estilos, cada uno con modo día y noche: **Campo**, **Sampler**, **Cristal**, **Metal**, **Ritmo**, **Neón** y **Papel**, más 12 paletas rápidas.
- Personalizar: un botoncito en cada pieza (pantalla, grados, círculo, pentagrama, teclas) para ajustar solo esa pieza; panel general con estilos, temas listos, color principal, colores suaves y movimiento. Paneles de vidrio esmerilado que escalan con el plugin y cambian de tamaño desde la esquina.
- Nueve sonidos: piano de cola, de concierto, suave, de bar, eléctrico, eléctrico con trémolo, celesta, de juguete y clavecín.
- Botón **Pedal** en pantalla: sostiene las notas sin pedal físico.
- Cada parte se puede separar en su propia ventana, minimizar y regresar a su lugar con "Restablecer".
- Metrónomo integrado con 6 sonidos suaves, tempo, compás y volumen (dentro del DAW sigue su tempo).
- Grabadora MIDI: guarda lo que tocas (también lo que tocas con el mouse) en Música > MidiCat > Grabaciones, listo para arrastrar a cualquier DAW; el archivo llega con la tonalidad en el nombre (por ejemplo "Sib mayor - 96 BPM").
- Grados con teclas de la compu (1 a 7) y con pads MIDI, que suenan y se graban.
- Pasa cualquier MIDI a audio (WAV) con el sonido elegido para usarlo como sample.
- Piano roll con la vista y los comandos de Ableton (notas cuadradas, lápiz que pinta siguiendo al mouse, doble clic, B, Cmd/Ctrl+D, E, U, 1-4, copiar con Option/Ctrl, zoom en la regla, velocity, deshacer, copiar/pegar).
- En la app sola: busca (en segundo plano, sin trabarse) y toca con tus propios instrumentos VST3 / AU.
- Presets propios de temas y de acomodos de ventanas.
- Conectar y desconectar MIDI sin interrumpir a Ableton.

## Descargar

En **Releases → MidiCat · última versión** están los instaladores:
`MidiCat-Instalador-Mac.pkg` y `MidiCat-Instalador-Windows.exe`
(y los `.zip` para instalar a mano, cada uno con su `LEEME.txt`).

## Compilar

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

JUCE 8 se descarga solo. GitHub Actions compila las versiones de Mac y Windows en cada cambio.

## Créditos y licencias

- Piano de cola: [Salamander Grand Piano](https://archive.org/details/SalamanderGrandPianoV3) de Alexander Holm (CC BY 3.0).
- Piano de concierto: [Splendid Grand Piano](https://github.com/sfzinstruments/SplendidGrandPiano), muestras de dominio público de Akai.
- Piano eléctrico: jRhodes3d - https://github.com/sfzinstruments/jlearman.jRhodes3d, de Jeff Learman (CC BY 4.0).
- Notación: fuente Bravura de Steinberg (SIL Open Font License).
- Tipografías: Zen Kaku Gothic New y Jersey 10 (SIL Open Font License).
- Hecho con [JUCE](https://juce.com) bajo AGPLv3. Este proyecto también se distribuye bajo AGPLv3 (ver `LICENSE`).
