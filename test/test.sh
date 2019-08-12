#!/bin/sh

do_test()
{
  ./bin/dfile --file-url  http://mirror.bit.edu.cn/apache//httpd/mod_ftp/mod_ftp-0.9.6-beta.tar.bz2 \
          --md5-url   https://www.apache.org/dist/httpd/mod_ftp/mod_ftp-0.9.6-beta.tar.bz2.md5
}

do_test

