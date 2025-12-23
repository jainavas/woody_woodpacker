# woody_woodpacker

Un packer simple para binarios ELF de 64 bits que cifra ejecutables y los hace auto-descifrables en tiempo de ejecución.

---

## 📋 Descripción

**woody_woodpacker** es un proyecto educativo de la escuela 42 que implementa un packer básico. Un packer es una herramienta que:

1. Toma un ejecutable como entrada
2. Cifra sus secciones de código
3. Inyecta un stub de descifrado
4. Genera un nuevo ejecutable que se auto-descifra al ejecutarse

El binario resultante (`woody`) es funcionalmente idéntico al original, pero su código está cifrado en disco y solo se descifra en memoria durante la ejecución.

### ¿Por qué existen los packers?

Históricamente, los packers se utilizaban para:
- **Evasión de antivirus**: El código cifrado no puede ser analizado estáticamente
- **Compresión**: Reducir el tamaño de ejecutables
- **Protección de propiedad intelectual**: Dificultar la ingeniería inversa

> ⚠️ **Nota ética**: Este proyecto es puramente educativo. El uso de packers para distribuir malware es ilegal.

---

## 🎯 Características

- ✅ Cifrado de la sección `.text` usando **RC4**
- ✅ Generación de claves aleatorias criptográficamente seguras
- ✅ Stub de descifrado inyectado en el binario
- ✅ Modificación del entry point para ejecutar el stub primero
- ✅ Preservación completa de la funcionalidad del programa original
- ✅ Soporte exclusivo para ELF de 64 bits

---

## 🏗️ Estructura del Proyecto

```
woody_woodpacker/
├── Makefile
├── README.md
├── include/
│   └── woody.h              # Declaraciones y estructuras principales
├── src/
│   ├── main.c               # Punto de entrada del programa
│   ├── elf_parser.c         # Parser del formato ELF
│   ├── rc4.c                # Implementación del algoritmo RC4
│   └── packer.c             # Lógica de empaquetado
└── stub/
    ├── stub.c               # Código del stub de descifrado
    └── stub.bin             # Stub compilado (generado)
```

---

## 🛠️ Compilación

### Requisitos

- GCC (compilador de C)
- GNU Make
- Sistema operativo Linux (x86-64)
- Librería estándar de C

### Instrucciones

```bash
# Clonar el repositorio (o extraer el proyecto)
cd woody_woodpacker

# Compilar el proyecto
make

# Esto genera:
# - woody_woodpacker (el packer principal)
# - stub/stub.bin (el stub de descifrado)
```

### Comandos del Makefile

```bash
make          # Compilar todo
make clean    # Limpiar objetos
make fclean   # Limpiar objetos y ejecutables
make re       # Recompilar desde cero
```

---

## 🚀 Uso

### Sintaxis

```bash
./woody_woodpacker <binario_ejecutable>
```

### Ejemplo básico

```bash
# Crear un programa de prueba
cat > test.c << 'EOF'
#include <stdio.h>
int main(void) {
    printf("Hello, World!\n");
    return 0;
}
EOF

# Compilar
gcc -o test test.c

# Empaquetar
./woody_woodpacker test

# Output esperado:
# [*] Found .text: offset=0x1000 size=123 bytes
# [*] Encrypting .text section...
# [+] .text encrypted successfully
# Generated key: A3F21D8EC47B09E844217F3C5D6A8E91
# [+] Packing completed! Output: woody

# Ejecutar el original
./test
# Hello, World!

# Ejecutar la versión empaquetada
./woody
# ....WOODY....
# Hello, World!
```

### Verificar el cifrado

```bash
# Comparar el código original vs cifrado
objdump -d test | grep -A 10 "<main>"
objdump -d woody | grep -A 10 "<main>"

# El código en woody estará cifrado (bytes aparentemente aleatorios)
```

---

## 📐 Arquitectura Técnica

### Flujo de Empaquetado

```
┌─────────────────────┐
│  Binario Original   │
│   (test.c → test)   │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  1. Parse ELF       │
│  - Leer headers     │
│  - Localizar .text  │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  2. Generar Clave   │
│  - /dev/urandom     │
│  - 16 bytes (128b)  │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  3. Cifrar .text    │
│  - Algoritmo RC4    │
│  - In-place XOR     │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  4. Inyectar Stub   │
│  - Añadir código    │
│  - Modificar entry  │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│   Binario Woody     │
│   (test → woody)    │
└─────────────────────┘
```

### Flujo de Ejecución

```
┌─────────────────────┐
│  Usuario: ./woody   │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Loader del SO      │
│  - Mapea woody      │
│  - Salta a entry    │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Stub ejecuta       │
│  1. mprotect(RWX)   │
│  2. Descifra .text  │
│  3. mprotect(RX)    │
│  4. printf woody    │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Salta a main()     │
│  - Código original  │
│  - Ejecución normal │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Programa termina   │
└─────────────────────┘
```

---

## 🔐 Algoritmo RC4

### ¿Por qué RC4?

- **Simplicidad**: ~50 líneas de código
- **Velocidad**: Cifrado por stream muy rápido
- **Simetría**: Misma operación para cifrar y descifrar
- **Histórico**: Usado en WEP, TLS 1.0, y packers reales (UPX)

> ⚠️ **Nota**: RC4 tiene vulnerabilidades conocidas y no debe usarse en aplicaciones de producción modernas. Este proyecto es educativo.

### Funcionamiento

```c
// Inicialización
uint8_t S[256];  // Estado interno
// Baraja S según la clave

// Cifrado/Descifrado
for (cada byte) {
    generar_byte_pseudoaleatorio();
    byte_cifrado = byte_original ^ byte_pseudoaleatorio;
}
```

### Propiedades clave

- **Determinista**: Misma clave → mismo flujo
- **Reversible**: XOR es su propia inversa
- **No requiere padding**: Funciona con cualquier tamaño

---

## 📚 Conceptos del Formato ELF

### Estructura de un ELF

```
┌─────────────────────────────┐
│  ELF Header (64 bytes)      │
│  ┌─────────────────────┐    │
│  │ Magic: 0x7F 'ELF'   │    │
│  │ Class: 64-bit       │    │
│  │ Entry: 0x401000     │◄───┼── Punto de entrada
│  └─────────────────────┘    │
├─────────────────────────────┤
│  Program Headers            │
│  ┌─────────────────────┐    │
│  │ LOAD (RX)           │    │
│  │ LOAD (RW)           │    │
│  └─────────────────────┘    │
├─────────────────────────────┤
│  .text (código)             │◄─── Aquí está tu main()
│  48 8D 3D ... (opcodes)     │    (esto se cifra)
├─────────────────────────────┤
│  .rodata (constantes)       │
├─────────────────────────────┤
│  .data (variables globales) │
├─────────────────────────────┤
│  Section Headers            │
└─────────────────────────────┘
```

### Secciones importantes

| Sección | Permisos | Contenido | ¿Se cifra? |
|---------|----------|-----------|------------|
| `.text` | R-X | Código ejecutable (funciones) | ✅ SÍ |
| `.rodata` | R-- | Strings y constantes | ❌ No |
| `.data` | RW- | Variables inicializadas | ❌ No |
| `.bss` | RW- | Variables sin inicializar | ❌ No |

---

## 🔧 Funciones Permitidas

Según el subject del proyecto:

### Manipulación de archivos
- `open`, `close`
- `read`, `write`
- `lseek`

### Mapeo de memoria
- `mmap`: Mapear archivo en memoria
- `munmap`: Liberar mapeo
- `mprotect`: Cambiar permisos de páginas

### Utilidades
- `malloc`, `free`
- `printf` (familia)
- `perror`, `strerror`
- `exit`

---

## 🐛 Debugging y Testing

### Herramientas útiles

```bash
# Ver estructura ELF
readelf -h woody          # Header
readelf -l woody          # Program headers
readelf -S woody          # Section headers

# Desensamblar código
objdump -d woody          # Ver si .text está cifrado
objdump -d -M intel woody # Sintaxis Intel (más legible)

# Comparar binarios
diff <(xxd test) <(xxd woody)

# Verificar que woody ejecuta correctamente
./test > original_output.txt
./woody > packed_output.txt
diff original_output.txt packed_output.txt
# No debería haber diferencias (excepto "....WOODY....")
```

### Tests recomendados

```bash
# Test 1: Programa simple
echo 'int main(){return 0;}' | gcc -x c - -o test1
./woody_woodpacker test1
./woody && echo "✓ Test 1 passed"

# Test 2: Con argumentos
cat > test2.c << 'EOF'
#include <stdio.h>
int main(int argc, char **argv) {
    for(int i = 0; i < argc; i++)
        printf("arg[%d]: %s\n", i, argv[i]);
    return 0;
}
EOF
gcc -o test2 test2.c
./woody_woodpacker test2
./woody arg1 arg2 arg3

# Test 3: Con salida de error
echo 'int main(){return 42;}' | gcc -x c - -o test3
./woody_woodpacker test3
./woody; echo "Exit code: $?"  # Debe ser 42
```

---

## 📊 Limitaciones Conocidas

### Restricciones actuales

- ❌ Solo ELF de 64 bits (no 32 bits)
- ❌ No soporta PIE (Position Independent Executable)
- ❌ No implementa compresión
- ❌ No cifra otras secciones (solo `.text`)
- ❌ No ofusca el stub de descifrado

### Por qué estas limitaciones

| Limitación | Razón |
|------------|-------|
| Solo 64-bit | Simplificar estructuras ELF (Elf64_* vs Elf32_*) |
| No PIE | PIE complica el cálculo de direcciones virtuales |
| Sin compresión | Requiere librerías externas (zlib) o algoritmos complejos |
| Solo .text | Las otras secciones no son ejecutables |

---

## 🎓 Conceptos Aprendidos

Al completar este proyecto, habrás trabajado con:

### Sistemas Operativos
- Formato de ejecutables (ELF)
- Mapeo de memoria (`mmap`)
- Protección de páginas (`mprotect`)
- Syscalls directas

### Seguridad
- Cifrado simétrico (RC4)
- Generación de claves aleatorias
- Self-modifying code
- Evasión de análisis estático

### Programación de bajo nivel
- Manipulación de binarios
- Aritmética de punteros
- Estructuras packed
- Entry points y control flow

---

## 🚧 Posibles Mejoras (Bonus)

### Ideas de extensión

1. **Soporte 32-bit**
   - Usar `Elf32_*` estructuras
   - Adaptar el stub para arquitectura i386

2. **Compresión**
   - Integrar zlib o lz4
   - Comprimir antes de cifrar

3. **Múltiples algoritmos**
   - AES-256
   - ChaCha20
   - Selección con flag `-a <algorithm>`

4. **Ofuscación del stub**
   - Cifrar el propio stub
   - Metamorfismo (cambiar el stub cada vez)

5. **Soporte para otros formatos**
   - PE (Windows executables)
   - Mach-O (macOS executables)

---

## 📖 Referencias y Recursos

### Documentación oficial
- [ELF Specification](https://refspecs.linuxfoundation.org/elf/elf.pdf)
- [System V ABI](https://wiki.osdev.org/System_V_ABI)
- `man elf`
- `man mmap`
- `man mprotect`

### Tutoriales recomendados
- [A Whirlwind Tutorial on Creating Really Teensy ELF Executables](http://www.muppetlabs.com/~breadbox/software/tiny/teensy.html)
- [ELF 101 (Practical Binary Analysis)](https://practicalbinaryanalysis.com/)
- [Linux Insides - ELF Loading](https://0xax.gitbooks.io/linux-insides/content/SysCall/linux-syscall-4.html)

### Herramientas
- `readelf`: Inspeccionar ELF
- `objdump`: Desensamblar
- `xxd`: Hex dump
- `strace`: Trace syscalls
- `gdb`: Debugger

---

## 🤝 Créditos

Proyecto desarrollado como parte del curriculum de la escuela 42.

### Autor
- **jainavas** - [42 Madrid](https://www.42madrid.com/)

### Agradecimientos
- Comunidad de 42 por el soporte y discusiones técnicas
- Recursos open-source de análisis de binarios

---

## 📄 Licencia

Este proyecto es de código abierto y está disponible bajo la licencia MIT.

```
MIT License

Copyright (c) 2024 jainavas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## ⚠️ Disclaimer Legal

Este software se proporciona únicamente con fines educativos y de investigación. El uso de packers para distribuir malware, evadir sistemas de seguridad legítimos, o cualquier actividad ilegal está estrictamente prohibido.

El autor no se hace responsable del uso indebido de este software. Los usuarios son responsables de cumplir con todas las leyes y regulaciones aplicables en su jurisdicción.

---

## 📞 Contacto

Para preguntas, sugerencias o reportar bugs:
- GitHub Issues: [Abrir un issue](#)
- Intra 42: `jainavas`

---

**¡Feliz hacking ético! 🎉**