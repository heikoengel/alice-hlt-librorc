FILE(REMOVE_RECURSE
  "fli.pdb"
  "fli.so"
)

# Per-language clean rules from dependency scanning.
FOREACH(lang)
  INCLUDE(CMakeFiles/fli.dir/cmake_clean_${lang}.cmake OPTIONAL)
ENDFOREACH(lang)
