# CellulOS

## Build

### Toolchain
Le projet se compile avec `x86_64-elf-g++`, compilé avec des options particulières car
les librairies intégrées à gcc sont compilées avec les options `-mno-red-zone` et `-mcmodel=large`.
Pour ce faire on peut suivre les instructions trouvées
[ici](osdev.org/GCC_Cross-Compiler) et [là](https://osdev.org/Libgcc_without_red_zone).
En particulier, après avoir appliqué la modification du second lien,
la configuration et la compilation de GCC (à partir du dossier build) suit la forme suivante :
```
../gcc-13.2.0/configure --target=x86_64-elf --disable-nls --enable-languages=c,c++ --without-headers --prefix=$PREFIX --enable-cxx-flags='-mcmodel=large' --enable-cxx-static-eh-pool --enable-libstdcxx-allocator=new --disable-hosted-libstdcxx

make all-gcc -j 8
make all-target-libgcc CFLAGS_FOR_TARGET='-g -O2 -mcmodel=large'
make all-target-libstdc++-v3  -j 8
make install-gcc 
make install-libgcc
make install-libcpp
make install-target-libstdc++-v3 
```
### Make appelle sudo
Pour construire une image test de système de fichier, le Makefile principal a besoin de monter un fichier
comme loopback. C'est une opération privilégiée par défaut pour le noyau; de la sorte, lors de la compilation,
il se peut que make demande un mot de passe via sudo. Il est bien sûr possible d'effectuer les opérations
manuellement (en commentant les règles du prérequis `makefat`).

## Organisation du dépôt
Les dossiers sont organisés comme suit :
- boot/ contient le bootstrapper de l'OS, qui initialise les structures du CPU et
s'assure de la transition dans le mode 64 bit.
- kernel/ contient le coeur de l'OS, dont ses sous-systèmes dans des sous-dossiers (/kernel/mem, /kernel/fs, etc.)
- drivers/ contient les drivers de l'OS, même s'ils sont statiquement insérés dans l'image de l'OS
(ce qui est toujours le cas). Chaque driver a un sous-dossier; les fichiers objets
obtenus pour chaque driver sont liés dans un unique fichier `drivers.o`.
- kstdlib/ contient une librairie C / C++ très minimale utilisée pour simplifier le code ailleurs.
Les fonctions définies ne sont souvent pas incluses dans l'espace de nom `std` afin d'éviter la confusion.
- stdinit/ contient l'implémentation des fichiers de démarrage/arrêt liés aux librairies incluses par gcc.
- userspace/ contient une demo du mode utilisateur.
