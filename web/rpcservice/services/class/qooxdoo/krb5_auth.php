<?php

class krb5_auth 
{
  private $krb_conn;
  private $realm;
  private $debug;

  public function __construct($krb_username, $keytab, $realm) 
  {
    $this->debug = false;
    $this->realm = $realm;
    $this->krb_conn = new KADM5($krb_username, $keytab, true); //use keytab=true
  }
        
  /* Add a new user. Returns true on success, false otherwise */
  public function add_user($username, $password)
  {
    /* Just incase some smartarse appends /admin to their handle
     * in an attempt to become a krb admin... */
    if(stristr($username, '/admin') === FALSE)
    { 
      try
      {
        $princ = new KADM5Principal($username);
        $this->krb_conn->createPrincipal($princ, $password);    
      } catch (Exception $e)
      {
        if ($this->debug) echo "$e\n";
        return false;
      }
      return true;     
    } else
    {
      if ($this->debug) echo "Attempt to create admin user stopped.";
      return false;
    }
  }
        
  public function delete_user($username)
  {
    try 
    {  
      $princ = $this->krb_conn->getPrincipal($username);
      $princ->delete();    
    } catch (Exception $e)
    {
      if ($this->debug) echo "$e\n";
      return false;
    }
    return true;
  }
        
  public function check_password($username, $password)
  {
    $ticket = new KRB5CCache();
    try
    {
      $ticket->initPassword($username . "@" . $this->realm, $password);
    } catch (Exception $e)
    {
      if ($this->debug) echo "$e\n";
      return false;
    } 
    return true;
  }
  
  public function change_password($username, $newpassword)
  {
    try
    {
      $princ = $this->krb_conn->getPrincipal($username);
      $princ->changePassword($newpassword);
    } catch (Exception $e)
    {
      if ($this->debug) echo "$e\n";
      return false;
    }
    return true;
  }

  /* Check if user exists in krb database. Returns:
   * TRUE  = user exists
   * FALSE = user doesn't exist
   * NULL  = something went wrong
   */
  function user_exists($username)
  {
    try
    {
      $this->krb_conn->getPrincipal($username);
    } catch (Exception $e)
    {
      if ($e->getMessage() == "Principal does not exist")
        return FALSE;
      else
        return NULL;
    }
    return TRUE;
  }
  
  function get_users()
  {
    $lst = array();
    foreach($this->krb_conn->getPrincipals() as $princ) 
    {
      $lst[] = strtolower(str_replace("@".$this->realm, "", $princ));
    }
    return $lst;    
  }
} 

?>
