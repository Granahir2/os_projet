# Organisation des FS

## Principes généraux

_Headers concernés : fs.hh, concepts.hh_

Une session sur un fichier est représentée par une instance d'un type dérivé de `filehandler`,
qui décrit les opérations faisables sur un fichier (lire, écrire, seek, etc.). La "librairie C"
`kstdlib` a pour convention `FILE* = filehandler*`.

Un système de fichier est représenté via un type dérivé de `fs`. Pour l'instant, `fs` ne
spécifie qu'une fonction virtuelle, `get_iterator()`. Celle-ci renvoie un _itérateur_ à travers
le système de fichier.

Cet itérateur est cette fois-ci un type dérivé de `dir_iterator`. L'opérateur `<<` fait traverser
le système de fichier par l'itérateur, un segment de chemin à la fois. Si le segment de chemin ferait
rentrer l'itérateur dans un fichier, l'opération n'a pas d'effet et renvoie `FILE_ENTRY`; elle renvoie
`LINK_ENTRY` s'il s'agit d'un lien, et ainsi de suite; `NP` représente une entrée non présente.
Les opérations `readlink(const char*)` et `open_file(const char*, int)` permettent à l'itérateur respectivement
de lire un lien (le lien n'est _pas_ suivi) ou d'ouvrir un fichier dans le dossier courant.
La fonction `get_canonical_path()` renvoie le chemin actuel de l'itérateur.


## Templates, concepts

_Headers concernés : templates.hh, concepts.hh_

Afin de préserver la sémantique des modes d'ouverture, les types dérivés de `filehandler`
sont nécessairement des instances de `perm_fh<T>` avec `T` implémentant en interne les opérations de fichier;
la lecture, l'écriture et _éventuellement_ le seek (si ce n'est pas implémenté, le fichier résultant renverra
EINVAL lors d'un seek). Dans l'avenir `stat()` sera aussi implémentable par `T`.

Dans le même esprit, une template `coh_dit<T>` est fournie pour générer des itérateurs à partir d'opérations
plus simple d'empilage et dépilage dans la hierarchie de dossiers.

Les concepts de `concepts.hh` décrivent précisément l'API que doit fournir `T` à chaque fois.

## En pratique

_Headers concernés : devfs.hh, devfs.cc_

Un exemple de FS implémenté est `devfs`, via les templates précédemment décrites. Il interagit avec
des fichiers définis par les drivers (pour l'instant seuls les ports série sont supportés).

Le système de fichiers `rootfs` est un peu particulier : c'est lui qui gère les sémantiques des points de
montage, de liens etc... c'est le système de fichier présent à la racine. Son implémentation ne passe
pas par `coh_dit` pour ces raisons.

