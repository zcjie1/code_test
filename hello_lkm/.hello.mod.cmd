savedcmd_/home/zcj/c_test/hello_md/hello.mod := printf '%s\n'   hello.o | awk '!x[$$0]++ { print("/home/zcj/c_test/hello_md/"$$0) }' > /home/zcj/c_test/hello_md/hello.mod
