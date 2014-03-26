FILE(REMOVE_RECURSE
  "librorc.pdb"
  "librorc.so"
  "librorc.so.1.0.1"
  "librorc.so.0"
)

# Per-language clean rules from dependency scanning.
FOREACH(lang)
  INCLUDE(CMakeFiles/rorc.dir/cmake_clean_${lang}.cmake OPTIONAL)
ENDFOREACH(lang)
