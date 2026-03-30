SHELL := /bin/bash
FILE=test_5_cat.txt

all: ci s21_cat
	


build: ci s21_cat




s21_cat: 
	gcc -Wall -Werror -Wextra s21_cat.c -o s21_cat 






cf:
	cp ../../materials/linters/.clang-format .




cn:
	cp ../../materials/linters/.clang-format .
	clang-format -n *.c




ci:
	cp ../../materials/linters/.clang-format .
	clang-format -i *.c 







tes:
	sh test_s21_cat.sh
	




tests:
	-diff <(./s21_cat $(FILE)) <(cat $(FILE))
	-diff <(./s21_cat -s $(FILE)) <(cat -s $(FILE))
	-diff <(./s21_cat --squeeze-blank $(FILE)) <(cat -s $(FILE))
	-diff <(./s21_cat -e $(FILE)) <(cat -e $(FILE))
	-diff <(./s21_cat -n $(FILE)) <(cat -n $(FILE))
	-diff <(./s21_cat --number $(FILE)) <(cat -n $(FILE))
	-diff <(./s21_cat -b $(FILE)) <(cat -b $(FILE))
	-diff <(./s21_cat --number-nonblank $(FILE)) <(cat -b $(FILE))
	-diff <(./s21_cat -t $(FILE)) <(cat -t $(FILE))
	-diff <(./s21_cat -e $(FILE)) <(cat -e $(FILE))
	-diff <(./s21_cat -v $(FILE)) <(cat -v $(FILE))
	-diff <(./s21_cat -en $(FILE)) <(cat -en $(FILE))
	-diff <(./s21_cat -eb $(FILE)) <(cat -eb $(FILE))
	-diff <(./s21_cat -senbtev $(FILE)) <(cat -senbtev $(FILE))




clean:
	rm -rf ./s21_cat ./.clang-format






rebuild: clean ci all


	 



	
