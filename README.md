# KOF '98 Music Select

Mod para **The King of Fighters '98 Ultimate Match Final Edition** (Steam, x64).
Cuando terminas de acomodar el orden del equipo y el combate va a empezar, aparece
**SELECT MUSIC**. La pista elegida sustituye la música de ese combate. El juego y sus
archivos no se modifican: el mod solo está en el proceso que arranca el lanzador, y lee
las pistas que ya vienen con tu copia.

## Uso

1. Abre Steam con la sesión iniciada.
2. Ejecuta `dist\Jugar KOF98 con musica.bat`, o mejor `Jugar KOF98 con selector.bat` del
   selector de controles: si `Kof98Bgm.dll` está junto a `KofVsLauncher.exe`, el lanzador
   carga los dos mods a la vez.
3. Al acabar el orden de los personajes, elige la música:

| Acción | Teclado | Mando |
|---|---|---|
| Mover | Arriba / Abajo | Cruceta o stick arriba / abajo |
| Cambiar de página | RePág / AvPág | Cruceta o stick izquierda / derecha |
| Elegir la canción o abrir el álbum | — | START o D |
| Canción al azar | — | START + D a la vez |
| Volver al álbum anterior | — | SELECT |

START + D tiene prioridad: si los dos se pulsan juntos (con 90 ms de margen) solo sale la
canción al azar, nunca también "elegir". Dentro de un álbum, el azar elige solo de ese
álbum y sus subcarpetas; en la raíz, de toda la biblioteca. D solo hace esto dentro de la
ventana de música; en combate sigue siendo el ataque de siempre. Mientras la ventana está
abierta el juego no recibe botones. Al moverte por la lista suena la previa de cada pista
con el reproductor del juego.

### Qué mandos controlan la ventana

Con el selector de controles (`KofVsSelector.dll`) cargado, la ventana solo obedece a los
dos mandos elegidos como P1 y P2, con su perfil de botones (D es el botón que tengas como D
en ese perfil). Si cambian los jugadores entre combates, la ventana sigue a los nuevos sin
hacer nada. Si un mando se desconecta, deja de contar; al reconectarlo y elegirlo otra vez
en el selector, vuelve a contar. Sin el selector, la ventana acepta cualquier mando XInput.

## Álbumes y canciones propias

La lista es un árbol de álbumes:

- `KOF 98 UM - Soundtrack`: las pistas del juego.
- Los álbumes incluidos (KOF 98 City Pop, KOF 99 Soundtrack, Street Fighter, ...).
- Cada carpeta dentro de `Music`, con sus subcarpetas como subálbumes.

`Music` puede estar junto a la DLL o junto al `.exe` del juego (se leen las dos). Los
archivos sueltos en la raíz de `Music` aparecen en el álbum `Otros`. Las carpetas y las
canciones se ordenan por nombre (`2` antes de `10`). Cada vez que se abre la ventana se
vuelve a leer `Music`, así que una canción nueva aparece sin reiniciar el juego. Las
carpetas vacías no aparecen. Los archivos que el juego no puede reproducir se ignoran y se
anotan en `kof98bgm.log`.

El juego solo reproduce OGG Vorbis estéreo a 44100 Hz (KOF 2002: 48000 Hz). Para convertir
una carpeta con mp3, flac, wav, m4a, etc. a los dos formatos de una vez:

```bat
dotnet run -c Release --project tools\TrackPrep -- --music "D:\MiMusica" --out98 "<carpeta KOF98>\Music" --out2002 "<carpeta KOF2002>\Music"
```

Conserva las subcarpetas (cada una será un álbum), busca el punto de loop de cada canción
y escribe el `.loop` al lado. Solo convierte lo nuevo o lo que cambió. Hace falta
`ffmpeg` en el PATH (o `--ffmpeg <ruta>`). Un `.loop` es un número de segundos desde el
inicio del archivo (`0.000000` = vuelve al principio); se puede editar a mano.

`Ctrl+Shift+F8` abre la pantalla antes, si quieres elegir durante la selección de personajes.

La última elección queda en `dist\kof98bgm.ini`. Lo que ocurrió en la sesión queda en `dist\kof98bgm.log`.

Para jugar sin el mod, cierra el juego y ábrelo desde Steam.

## Canciones nuevas

Las pistas de `nuevas canciones` se preparan con `tools\TrackPrep`. El manifiesto
`tools\TrackPrep\tracks.txt` dice qué archivo usar, con qué nombre de pista y con qué
título. La herramienta busca dónde se repite cada canción y la corta ahí, para que
haga loop sin salto; si no encuentra repetición, la canción vuelve al principio.
Deja los `.ogg` en `build\tracks\kof98` (44.1 kHz) y `build\tracks\kof2002`
(48 kHz con su `.loop`), y escribe `src\extra_tracks.h`. Copia ese archivo también a
`D:\KOF2002-Music-Select\src` antes de compilar esa DLL.

```bat
dotnet run -c Release --project tools\TrackPrep -- --ffmpeg ffmpeg.exe --songs "nuevas canciones" --manifest tools\TrackPrep\tracks.txt --out build\tracks --header src\extra_tracks.h
```

## Compilar

Hace falta Visual Studio 2022 y el SDK de .NET 9.

```bat
build.bat
```
