<?php
require "CNHDBAccess.php";

function db_link()
{
  return new CNHDBAccess('localhost', '<WEB USERNAME>', '<WEB PASSWORD>', 'instrumentation');
}

function db_link2()
{
  $link = mysqli_connect("localhost","<WEB USERNAME>", "<WEB PASSWORD>", "instrumentation");
  mysqli_set_charset($link, "utf8");
  return $link;
}

function db_pdo_link()
{
  $link = new PDO('mysql:host=localhost;dbname=instrumentation', '<WEB USERNAME>', '<WEB PASSWORD>');
  $link->exec("set names utf8");
  return $link;
}

function krb_auth()
{
  $krb5 = new krb5_auth("hms_test/web", "/PATH/TO/KEYTAB", "NOTTINGTEST.ORG.UK");
  return $krb5;
}


?>