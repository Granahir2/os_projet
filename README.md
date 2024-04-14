# Projet d'OS

## Build
Le projet se compile avec `x86_64-elf-g++`, compilé avec des options particulières car
les librairies intégrées à gcc sont compilées avec les options `-mno-red-zone` et `-mcmodel=large`.
Pour ce faire on peut suivre les instructions trouvées
[ici](osdev.org/GCC_Cross-Compiler) et [là](https://osdev.org/Libgcc_without_red_zone) en ajoutant les 
opérations suivantes :
- ajouter lors de la configuration de gcc l'option `--enable-cxx-flags='-mcmodel=large'`
- lors de `make all-target-libgcc`, ajouter l'option `CFLAGS_FOR_TARGET='-g -O2 -mcmodel=large'`
- une fois gcc et libgcc compilés, exécuter `make all-target-libstdc++-v3` pour build libstdc++

Une fois les outils de compilation obtenus, `make` construit une ISO de l'OS, `make test` exécute un
test de l'image via QEMU. `make debug` initie une session de débogage via gdb.

## Organisation du dépôt
Les dossiers sont organisés comme suit :
- boot/ contient le bootstrapper de l'OS, qui initialise les structures du CPU et
s'assure de la transition dans le mode 64 bit.
- kernel/ contient le coeur de l'OS, dont ses sous-systèmes dans des sous-dossiers (/kernel/mem, /kernel/fs, etc.)
- drivers/ contient les drivers de l'OS, même s'ils sont statiquement insérés dans l'image de l'OS
(ce qui est pour l'instant toujours le cas). Chaque driver a un sous-dossier; les fichiers objets
obtenus pour chaque driver sont liés dans un unique fichier `drivers.o`.
- kstdlib/ contient une librairie C / C++ très minimale utilisée pour simplifier le code ailleurs.
Les fonctions définies ne sont souvent pas incluses dans l'espace de nom `std` afin d'éviter la confusion.
- stdinit/ contient l'implémentation des fichiers de démarrage/arrêt liés aux librairies incluses par gcc.

