#!/bin/bash

export DEBIAN_FRONTEND=noninteractive

cd /vagrant
git submodule update --init --recursive

# Create MySQL database
mysql -uroot -proot -e "create database instrumentation"
mysql -uroot -proot -e "grant select on instrumentation.* to 'inst_run'@'localhost' identified by 'inst_password'"

# Create HMS MySQL account
mysql -uroot -proot -e "GRANT ALL ON *.* TO 'hms'@'localhost' IDENTIFIED BY '' WITH GRANT OPTION"
mysql -uroot -proot -e "FLUSH PRIVILEGES"


apachectl restart

cd /vagrant
make
php db/db_load.php ALL

# TODO: need to fix this in db_load.php
mysql -uroot -proot instrumentation -e "GRANT EXECUTE ON FUNCTION fn_check_permission TO 'inst_run'@'localhost'"

cp web/db.php.template web/db.php
sed -i -e 's/<WEB USERNAME>/inst_run/g' web/db.php
sed -i -e 's/<WEB PASSWORD>/inst_password/g' web/db.php
sed -i -e 's/\/PATH\/TO\/KEYTAB/\/config\/nhweb.keytab/g' web/db.php
sed -i -e 's/hms_test\/web/inst\/web/g' web/db.php
chmod a+rw /config/nhweb.keytab 

if [ ! -f 'qooxdoo-4.0.1-sdk.zip' ]; then
    cp /downloads/qooxdoo-4.0.1-sdk.zip .
    unzip qooxdoo-4.0.1-sdk.zip
fi
make web2

rm -rf /var/www/html/
ln -s /vagrant/website/public/ /var/www/html

echo "Setting the password of all dummy accounts to be \"password\""...
mysql -uroot -proot instrumentation <<<"select lower(username) from members where username is not null and username != 'Admin'" |
while IFS='\n' read USERNAME
do
  kadmin.local -q "addprinc -pw password $USERNAME" > /dev/null 2>&1
done
echo "...Done"

echo "alias sql=\"mysql -proot -uroot instrumentation\"" > /home/vagrant/.bash_aliases

echo ""
echo "------------------------------------------------------------------------"
echo " **** nh-web should now be running at http://192.168.33.10/nhweb/ **** "
echo " **** Login using username=Admin, password=admin                  **** "
echo "      (all other accounts have the password \"password\")"
echo ""
echo "MySQL:  username = root,        password = root"
echo "kadmin: username = admin/admin, password = admin"
echo "kadmin: username = vagrant      password = vagrant"
echo ""
echo "Once connected, run 'sql' to start an SQL session in the HMS database, "
echo "and 'kadmin' administer the password database (password=vagrant)"
echo ""
echo "You can access the database at http://localhost:8080/phpmyadmin/"
echo "------------------------------------------------------------------------"
echo ""
