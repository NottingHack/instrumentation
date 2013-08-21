<?php

/* Wiki authentication plugin. This belongs in the wiki extensions folder, not on holly */

$bDebug = false;

$sLogFile = "/var/www/mediawiki-1.19.2/log/log.txt";
$url = 'https://lspace.nottinghack.org.uk/wiki/wikiauth.php';
$secret = "REDACTED";
$salt = 'REDACTED'; 


function writeMsg($sMsg) 
{
  global $sLogFile;
  global $bDebug;
  
  if ($bDebug)
  {
    $sOutput = date("M d H:i:s") . ": " . $sMsg . "\n";
    file_put_contents($sLogFile, $sOutput, FILE_APPEND);
  }
}


class NHAuth extends AuthPlugin
{
  var $uname = "";
  var $realname = "";
  var $email = "";
  
  function __construct() 
  {
    writeMsg("__construct()");
  }  
  
   function __destruct() 
   {
     writeMsg("__destruct() uname=[" . $this->uname . "]");
   }  
  
  /**
   * Check whether there exists a user account with the given name.
   * The name will be normalized to MediaWiki's requirements, so
   * you might need to munge it (for instance, for lowercase initial
   * letters).
   *
   * @param $username String: username.
   * @return bool
   */
  public function userExists( $username ) 
  {
    writeMsg("userExists($username)");
    return true;
  }

  /**
   * Check if a username+password pair is a valid login.
   * The name will be normalized to MediaWiki's requirements, so
   * you might need to munge it (for instance, for lowercase initial
   * letters).
   *
   * @param $username String: username.
   * @param $password String: user password.
   * @return bool
   */
  public function authenticate( $username, $password ) 
  {
    writeMsg("authenticate( $username, <password> )");

   // lets not authenticate local users against the members database in case of collisions 
   if ($this->is_local_user($username))
      return false;
    return $this->check_password($username, $password);
  }

  /**
   * Modify options in the login template.
   *
   * @param $template UserLoginTemplate object.
   * @param $type String 'signup' or 'login'. Added in 1.16.
   */
  public function modifyUITemplate( &$template, &$type ) 
  {
    $template->set('usedomain', false );
  }

  /**
   * Set the domain this plugin is supposed to use when authenticating.
   *
   * @param $domain String: authentication domain.
   */
  public function setDomain( $domain ) 
  {
    $this->domain = $domain;
  }

  /**
   * Check to see if the specific domain is a valid domain.
   *
   * @param $domain String: authentication domain.
   * @return bool
   */
  public function validDomain( $domain ) 
  {
    return true;
  }

  /**
   * When a user logs in, optionally fill in preferences and such.
   * For instance, you might pull the email address or real name from the
   * external user database.
   *
   * The User object is passed by reference so it can be modified; don't
   * forget the & on your function declaration.
   *
   * @param $user User object
   * @return bool
   */
  public function updateUser( &$user ) 
  {
    writeMsg("updateUser('" . $user->getName() . "')");

    if ($user->getName() == $this->uname)
    {
      $user->setRealName($this->realname);
      $user->setEmail($this->email);
      $user->confirmEmail();
      $user->mPassword = "#"; /* invalid password hash - i.e. no local password */
      $user->saveSettings();
      writeMsg("updateUser saved settings");      
    }
    return true;
  }

  /**
   * Return true if the wiki should create a new local account automatically
   * when asked to login a user who doesn't exist locally but does in the
   * external auth database.
   *
   * If you don't automatically create accounts, you must still create
   * accounts in some way. It's not possible to authenticate without
   * a local account.
   *
   * This is just a question, and shouldn't perform any actions.
   *
   * @return Boolean
   */
  public function autoCreate() 
  {
    writeMsg("autoCreate()");
    return true;
  }

  /**
   * Allow a property change? Properties are the same as preferences
   * and use the same keys. 'Realname' 'Emailaddress' and 'Nickname'
   * all reference this.
   *
   * @param $prop string
   *
   * @return Boolean
   */
  public function allowPropChange( $prop = '' ) 
  {
    writeMsg("allowPropChange($prop)");
    
    switch($prop)
    {
      case 'realname':
      case 'emailaddress':
        return false;
        break;
      default:
        return true;
    }
  }

  /**
   * Can users change their passwords?
   *
   * @return bool
   */
  public function allowPasswordChange() 
  {
    writeMsg("allowPasswordChange()");
    return true;  /* Nessesary to allow local account creation to work :( */
  }

  /**
   * Set the given password in the authentication database.
   * As a special case, the password may be set to null to request
   * locking the password to an unusable value, with the expectation
   * that it will be set later through a mail reset or other method.
   *
   * Return true if successful.
   *
   * @param $user User object.
   * @param $password String: password.
   * @return bool
   */
  public function setPassword( $user, $password ) 
  {
    writeMsg("setPassword($user, <password> )");
    return true;
  }

  /**
   * Update user information in the external authentication database.
   * Return true if successful.
   *
   * @param $user User object.
   * @return Boolean
   */
  public function updateExternalDB( $user ) 
  {
    writeMsg("updateExternalDB()");
    return false;
  }

  /**
   * Check to see if external accounts can be created.
   * Return true if external accounts can be created.
   * @return Boolean
   */
  public function canCreateAccounts() 
  {
    writeMsg("canCreateAccounts()");
    return false;
  }

  /**
   * Add a user to the external authentication database.
   * Return true if successful.
   *
   * @param $user User: only the name should be assumed valid at this point
   * @param $password String
   * @param $email String
   * @param $realname String
   * @return Boolean
   */
  public function addUser( $user, $password, $email = '', $realname = '' ) 
  {
    writeMsg("addUser($user, <password>, $email, $realname)");
    
    /* Need to return true here, to allow local users to be added by an admin */
    return true;
  }

  /**
   * Return true to prevent logins that don't authenticate here from being
   * checked against the local database's password fields.
   *
   * This is just a question, and shouldn't perform any actions.
   *
   * @return Boolean
   */
  public function strict() 
  {
    writeMsg("strict()");
    return false;
  }

  /**
   * Check if a user should authenticate locally if the global authentication fails.
   * If either this or strict() returns true, local authentication is not used.
   *
   * @param $username String: username.
   * @return Boolean
   */
  public function strictUserAuth( $username ) 
  {
    writeMsg("strictUserAuth( $username )");
    return false;
  }

  /**
   * When creating a user account, optionally fill in preferences and such.
   * For instance, you might pull the email address or real name from the
   * external user database.
   *
   * The User object is passed by reference so it can be modified; don't
   * forget the & on your function declaration.
   *
   * @param $user User object.
   * @param $autocreate Boolean: True if user is being autocreated on login
   */
  public function initUser( &$user, $autocreate = false ) 
  {
    writeMsg("initUser [$autocreate]");
  }

  /**
   * If you want to munge the case of an account name before the final
   * check, now is your chance.
   */
  public function getCanonicalName( $username ) 
  {
    writeMsg("getCanonicalName( $username )");

    return $username;
  }

  /**
   * Get an instance of a User object
   *
   * @param $user User
   *
   * @return AuthPluginUser
   */
  public function getUserInstance( User &$user ) 
  {
    writeMsg("getUserInstance");

    return new AuthPluginUser( $user );
  }

  /**
   * Get a list of domains (in HTMLForm options format) used.
   *
   * @return array
   */
  public function domainList() 
  {
    writeMsg("domainList()");
    return array();
  }

  /* Test if $username is a local user. Assume if the user exists in the mediawiki database *
   * and had a valid password set, that they're local                                       */
  function is_local_user($username)
  {
    $user = User::newFromName($username, 'creatable');
    if ($user == false) // means the username is invalid (not just doesn't exist)
    {
      writeMsg("is_local_user> invalid username");
      return false;
    }

    $user->load();

    if ($user->mId == "0") // user doesn't exist
    {
      writeMsg("is_local_user> user doesn't exist");
      return false;
    }

    if ($user->mPassword == "#") /* invalid password hash, as set by this plugin on account creation */
    {
      writeMsg("is_local_user> invalid hash - user not local");
      return false;
    } else
    {
      writeMsg("is_local_user> Not obviously invald password hash - so local user");
      return true;
    }
  }

  function check_password($username, $password)
  {
    $data = array('function'=>'login',
                  'username'=>$username,
                  'password'=>$password);

    $result = $this->hms_query($data);

    $granted = false;
    if ($result != FALSE)
    {
      if (isset($result['access_granted']))
      {
        if ($result['access_granted'] == true)
        {
          $granted = true;
          $this->realname = $result['name'];
          $this->email = $result['email'];
          $this->uname = $username;
        }
      }  
    }
    
    if ($granted)
      writeMsg("check_password($username) => Access granted");
    else
      writeMsg("check_password($username) => Access denied");
    return $granted;
  }
  
  function hms_query($data)
  {
    global $url;
    global $secret;
    global $salt;

    $data['hash']  = $secret;          
    $data['hash'] = crypt(json_encode($data), $salt);

    $query_string = http_build_query($data);

    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, $url);
    curl_setopt($ch, CURLOPT_POST, count($data));
    curl_setopt($ch, CURLOPT_POSTFIELDS, $query_string);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
    curl_setopt($ch, CURLOPT_FAILONERROR, TRUE);
    curl_setopt($ch, CURLOPT_CONNECTTIMEOUT, 4); // 4 second timeout 
    curl_setopt($ch, CURLOPT_CAINFO, dirname(__FILE__) . '/RapidSSL_CA_bundle.pem'); 
  //curl_setopt($ch, CURLOPT_SSLVERSION,3);

    $result = curl_exec($ch);

    if ($result == FALSE)
    {
      writeMsg("hms_query> curl_exec failed!");      
      writeMsg("curl_error=" . curl_error($ch));
      return FALSE;
    }      

    $res = json_decode($result, true);

    curl_close($ch); 
    
    writeMsg("hms_query> result = [" . print_r($res, true));
    return $res;
  }
}
 
