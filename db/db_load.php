<?php

/* Assumptions:
 *  1) For Stored Procedures, there is one SP per file, and the 
 *     filename is the same as the SP name (plus ".sql"), and all
 *     SPs are named "sp_...".
 * 
 *  2) For tables, there is one table per file, and the filename
 *     is "tb_<table name>.sql"
 */ 

class ctable
{
  public $filepath;
  public $loaded;
  public $error;
}

$config_file = "database.conf";
$base_path = __DIR__ . "/";

$aConfig = parse_ini_file($config_file, true);
if (!$aConfig)
  die("Error reading config file [" . $config_file . "]!\n");

$mysql_server = $aConfig['mysql']['server'];
$mysql_dbase = $aConfig['mysql']['database'];
$mysql_user = $aConfig['mysql_admin']['username'];
$mysql_passwd = $aConfig['mysql_admin']['password'];

$oDB = new mysqli($mysql_server, $mysql_user, $mysql_passwd, $mysql_dbase);

if ($oDB->connect_errno) 
  die("Failed to connect to database: [" . $mysqli->connect_error . "]\n");


/* Is database empty? */
if (($tc = table_count($oDB)) === FALSE)
  die("Error counting tables: [" . $oDB->error . "]\n");
if ($tc > 0)
  $db_empty = FALSE;
else
  $db_empty = TRUE;

if (isset($argv[1]))
{
  $filename = $argv[1];
  
  if ($filename == "ALL")
  {
    if (!$db_empty)
    {
      echo "\nWARNING: database [$mysql_dbase] is not empty!!!\n";
      echo "Really drop/recreate all TABLES and SPs? (yes/no): ";
      if (fgets(STDIN) != "yes\n")
        exit();
    }

    load_tables($base_path . "database/schema/", $aConfig, $oDB);

    // Load SPs
    $path = $base_path . "database/procedures/";
    $aFileList = scandir($path);
    foreach($aFileList as $file) 
    {
      if (is_sp($file))
        load_file($path . $file, $aConfig, $oDB);
    }

    // Load inital data
    load_inital_data($base_path, $aConfig, $oDB);
  }
  else if ($filename == "DATA")
  {
    load_inital_data($base_path, $aConfig, $oDB);
  }
  else if (is_table($filename))
  {
    if (file_exists(realpath($filename)))
      $filenamepath = realpath($filename);
    else if (file_exists(realpath($base_path . "database/schema/" . $filename)))
      $filenamepath = realpath($base_path . "database/schema/" . $filename);
    else
    {
      echo "Not found!\n";
      exit();
    }

    $table_name = basename($filenamepath, ".sql");
    $table_name = str_ireplace("tb_" , "", $table_name);

    if (table_exists($oDB, $aConfig, $table_name))
    {
      echo "\nWARNING: table [$table_name] in [$mysql_dbase] exists!\n";
      echo "Drop/recreate? (yes/no): ";
      if (fgets(STDIN) != "yes\n")
        exit();
    }

    load_file($filenamepath, $aConfig, $oDB);
  } 
  else if (is_sp($filename))
  {
    if (file_exists($filename))
    load_file($filename, $aConfig, $oDB);
    else
      load_file($base_path . "database/procedures/" . $filename, $aConfig, $oDB);
  } else
  {
    // Not an SP, not a table. What is it?
    die("\nUnknown object type: [$filename]\n");
  }
}
else
{
  echo "\nNo file specified. Load all SPs / Functions? (yes/no): ";
  $input = fgets(STDIN);
  if ($input == "yes\n")
  {
    $path = $base_path . "database/procedures/";
    $aFileList = scandir($path);
    foreach($aFileList as $file) 
    {
      if (is_sp($file))
        load_file($path . $file, $aConfig, $oDB);
    }
  }
}
  
  
function load_file($file, $aConfig, $oDB)
{
  $mysql_server = $aConfig['mysql']['server'];
  $mysql_dbase = $aConfig['mysql']['database'];
  $mysql_user = $aConfig['mysql_admin']['username'];
  $mysql_passwd = $aConfig['mysql_admin']['password'];
  $mysql_runtime_user = $aConfig['mysql_runtime']['username'];

  echo "Load [$file]...";
    
  $cmd = "mysql -u$mysql_user -p$mysql_passwd -h$mysql_server $mysql_dbase < $file";
  exec ($cmd, $out, $retval);
  if ($retval != 0)
    die("\n>>FAILED<<\n");
  
  // Now grant execute permission to the runtime account
  if (is_sp($file))
  {
    $spname = basename($file, ".sql");
    if (substr($spname, 0, 3) == "fn_")
      $query = "GRANT EXECUTE ON FUNCTION $spname TO '$mysql_runtime_user'@'localhost'";
    else
      $query = "GRANT EXECUTE ON PROCEDURE $spname TO '$mysql_runtime_user'@'localhost'";

    if ($oDB->query($query) === TRUE)
      echo "OK";
    else
    {
      echo "Error: [" . $oDB->error . "]\n";
      die(">>FAILED<<\n");
    }
  } else
    echo "OK";
  
  echo "\n";
}

function load_table(&$table, $aConfig, $oDB)
{
  $mysql_server = $aConfig['mysql']['server'];
  $mysql_dbase = $aConfig['mysql']['database'];
  $mysql_user = $aConfig['mysql_admin']['username'];
  $mysql_passwd = $aConfig['mysql_admin']['password'];
  $mysql_runtime_user = $aConfig['mysql_runtime']['username'];

  echo "Load [$table->filepath]...";
    
  $cmd = "mysql -u$mysql_user -p$mysql_passwd -h$mysql_server $mysql_dbase < " . $table->filepath . " 2>&1";
  exec ($cmd, $out, $retval);
  if ($retval != 0)
  {
    echo "Failed\n";
    $table->loaded = FALSE;
    $table->error = $out;
    return;
  }
  else
  {
    echo "Ok\n";
    $table->loaded = TRUE;
    return;
  }
}

function is_sp($filename)
{
  if (preg_match ("/(sp|fn)_+[^\.]+.sql$/", $filename) == 1)
    return TRUE;
  else
    return FALSE;
}

function is_table($filename)
{
  if (preg_match ("/tb_+[^\.]+.sql$/", $filename) == 1)
    return TRUE;
  else
    return FALSE; 
}

function is_data($filename)
{
  if (preg_match ("/data_+[^\.]+.sql$/", $filename) == 1)
    return TRUE;
  else
    return FALSE;
}

function table_count($oDB)
{
  if ($oResult = $oDB->query("show tables")) 
  {
    $num_rows = $oResult->num_rows;
    $oResult->close();
    return $num_rows;
  } else
    return FALSE;
}

function table_exists($oDB, $aConfig, $table_name)
{
  $bRet = TRUE;
  $mysql_dbase = $aConfig['mysql']['database'];
  $query = "select count(*) as c " .
           "from information_schema.tables " .
           "where table_schema = '$mysql_dbase' " . 
           "and table_name = '$table_name'";

  $oResult = $oDB->query($query);
  if($oResult)
  {
    $oRow = $oResult->fetch_object();
    {
      if ($oRow->c == 0)
        $bRet = FALSE;
    } 
    $oResult->close();
  } else
    die("Error checking if [$table_name] exists!\n");
  return $bRet;
}

function load_tables($path, $aConfig, $oDB)
{
  /* Due to FKs, the tables have to be loaded in a specific order. Instead of
     figuring this order out, just make repeated attemps to load all tables.
   */

  // Load tables
  $aFileList = scandir($path);
  $aTableList = array();

  foreach($aFileList as $file)
  {
    if (is_table($file) || is_data($file))
    {
      $oTable = new ctable();
      $oTable->filepath = $path . $file;
      $oTable->loaded = FALSE;
      $aTableList[] = $oTable;
    }
  }

  // Make up to 5 attempts to create each table (only retry if it failed to create the previous time)
  for ($pass = 0; $pass < 5; $pass++)
    foreach($aTableList as $table)
      if ($table->loaded == FALSE)
        load_table($table, $aConfig, $oDB);

  // List out tables that didn't create even after a few attempts
  echo "*** Failure list: ***\n";
  foreach($aTableList as $table)
  {
    if ($table->loaded == FALSE)
    {
      echo $table->filepath . ": ";
      foreach ($table->error as $err)
        echo "$err ";
      echo "\n";
    }
  }
  echo "*********************\n";
}

function load_inital_data($base_path, $aConfig, $oDB)
{
  // reuse table loader for this
  load_tables($base_path . "database/data/", $aConfig, $oDB);
}

?>
