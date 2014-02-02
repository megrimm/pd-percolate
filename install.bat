mkdir c:\pd\externs\PeRColate
cd PeRColate_help
copy *.* c:\pd\doc\5.reference
cd ..
copy percolate.dll c:\pd\externs\PeRColate
copy README.txt c:\pd\externs\PeRColate
copy PeRColate_manual.pdf c:\pd\externs\PeRColate
echo start Pd with flag "-lib c:\pd\externs\percolate\percolate"
