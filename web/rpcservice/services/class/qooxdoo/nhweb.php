<?php


require_once "server/lib/JSON.phps";

function db_link()
{
  return mysqli_connect("localhost","nh-web", "nh-web", "instrumentation");
}
    
function check_permission ($member_id, $permission_code)
{
  $link = db_link();
  $c = 0;
  if ($stmt = mysqli_prepare($link, "select fn_check_permission(?, ?) as p;")) 
  {
    mysqli_stmt_bind_param($stmt, "is", $member_id, $permission_code);
    if (mysqli_stmt_execute($stmt))
    {
      mysqli_stmt_bind_result($stmt, $c);
      mysqli_stmt_fetch($stmt);
      mysqli_stmt_close($stmt); 
    } else
      mysqli_stmt_close($stmt);    
  } else
  {
    mysqli_close($link);
    return false;
  }
  
  mysqli_close($link);
  if ($c == 1)
    return true;
  else
    return false;
}  


class class_nhweb extends ServiceIntrospection
{
  
    function method_prelogin($params, $error)
    {
        if (count($params) != 1)
        {
            $error->SetError(JsonRpcError_ParameterMismatch,
                             "Expected 1 parameter; got " . count($params));
            return $error;
        }


        $link = db_link();
        $ret = $this->sp_web_prelogin($link, $params[0]);
        mysqli_close($link);
        return $ret;
    }

    /**
     * Sink all data and never return.
     *
     * @param params
     *   An array containing the parameters to this method (none expected)
     *
     * @param error
     *   An object of class JsonRpcError.
     *
     * @return
     *   "Never"
     */
    function method_login($params, $error)
    {
        if (count($params) != 2)
        {
            $error->SetError(JsonRpcError_ParameterMismatch, "Expected 2 parameters; got " . count($params));
            return $error;
        }

        $link = db_link();
        $ret = $this->sp_web_login($link, $params[0], $params[1], $member_id);
        if ($ret)
        {
          session_regenerate_id(true);  
          session_start();
          $_SESSION['handle']    = $params[0];
          $_SESSION['member_id'] = $member_id;        
        }        
        mysqli_close($link);
        return $ret;
    }

    function method_getvendconfig($params, $error)
    {
        if (!isset($_SESSION['handle']))
          die("Not logged in");

        if (!check_permission($_SESSION['member_id'], "VIEW_VEND_CONFIG"))
        { 
          $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (VIEW_VEND_CONFIG)");
          return $error;          
        }
        
        $link = db_link();
        $result = mysqli_query($link, "select coalesce(vr.loc_name, '') as Position, coalesce(p.shortdesc, '') as Product from vmc_ref vr left outer join vmc_state vs on vr.vmc_ref_id = vs.vmc_ref_id left outer join products p on vs.product_id = p.product_id order by vr.loc_name");
        $rows = array();
        while($row = mysqli_fetch_assoc($result)) 
        {
          $rows[] = array($row["Position"], $row["Product"]);
        }
        mysqli_close($link);
        return json_encode($rows);
    }
    
    function method_productlistbox($params, $error)
    {
        if (!isset($_SESSION['handle']))
          die("Not logged in");

        if (!check_permission($_SESSION['member_id'], "VIEW_PRODUCTS"))
        { 
          $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (VIEW_PRODUCTS)");
          return $error;          
        }        
        
        $link = db_link();
        $result = mysqli_query($link, "select product_id, shortdesc from products order by shortdesc");
        $rows = array();
        while($row = mysqli_fetch_assoc($result)) 
        {
          $rows[] = array($row["product_id"], $row["shortdesc"]);
        }
        mysqli_close($link);
        return json_encode($rows);
    }

    function method_productlist($params, $error)
    {
        if (!isset($_SESSION['handle']))
          die("Not logged in");
        
        if (!check_permission($_SESSION['member_id'], "VIEW_PRODUCTS"))
        { 
          $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (VIEW_PRODUCTS)");
          return $error;          
        }        

        $link = db_link();
        $result = mysqli_query($link, "
          select 
            p.product_id, 
            p.shortdesc, 
            concat('£', cast((p.price/100) as decimal(20,2))) as price,
            (select count(*) from transactions t where t.product_id = p.product_id) as purchases
          from products p
          order by shortdesc;");
        $rows = array();
        while($row = mysqli_fetch_assoc($result)) 
        {
          $rows[] = array($row["product_id"], $row["shortdesc"], $row["price"], $row["purchases"]);
        }
        mysqli_close($link);
        return json_encode($rows);
    }

    function method_productdetails($params, $error)
    {
        if (!isset($_SESSION['handle']))
          die("Not logged in");
        
        if (!check_permission($_SESSION['member_id'], "VIEW_PRD_DETAIL"))
        { 
          $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (VIEW_PRD_DETAIL)");
          return $error;          
        }        

        $link = db_link();

        $product_id = $params[0];

        // Get result
        if ($stmt = mysqli_prepare($link, "select price, barcode, available, shortdesc, longdesc from products where product_id = ?")) 
        {
          mysqli_stmt_bind_param($stmt, "i", $product_id);
          mysqli_stmt_execute($stmt);
          mysqli_stmt_bind_result($stmt, $price, $barcode, $available, $shortdesc, $longdesc);
          mysqli_stmt_fetch($stmt);
          mysqli_stmt_close($stmt);    
        } else
        {
          return "";
        }
      
        return json_encode(array($price, $barcode, $available, $shortdesc, $longdesc));
    }    
        

    function method_setvendprd($params, $error)
    {
        if (!isset($_SESSION['handle']))
          die("Not logged in");
        
        if (!check_permission($_SESSION['member_id'], "UPD_VEND_CONFIG"))
        { 
          $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (UPD_VEND_CONFIG)");
          return $error;          
        }        

        $link = db_link();
        $ret = $this->sp_web_set_vendprd($link, $_SESSION['member_id'], $params[0], $params[1]);
        mysqli_close($link);
        return $ret;
    }

    function method_updateproduct($params, $error)
    {
        if (!isset($_SESSION['handle']))
          die("Not logged in");
        
        if (!check_permission($_SESSION['member_id'], "ADD_UPD_PRODUCT"))
        { 
          $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (ADD_UPD_PRODUCT)");
          return $error;          
        }        

        $link = db_link();
        $ret = $this->sp_web_updateproduct($link, $_SESSION['member_id'], $params[0], $params[1], $params[2], $params[3], $params[4], $params[5]);
        mysqli_close($link);
        return $ret;
    }


    function method_vendlog_rowcount($params, $error)
    {
      
      if (!isset($_SESSION['handle']))
        die("Not logged in");
  
      if (!check_permission($_SESSION['member_id'], "VIEW_VEND_LOG"))
      { 
        $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (VIEW_VEND_LOG)");
        return $error;          
      }

      $link = db_link();
      if ($stmt = mysqli_prepare($link, "select count(*) as c from vend_log")) 
      {
        mysqli_stmt_execute($stmt);
        mysqli_stmt_bind_result($stmt, $c);
        mysqli_stmt_fetch($stmt);
        mysqli_stmt_close($stmt);    
          
        if (is_null($c))
        {
          return "";
        }
        return $c;
      }
    }


    function method_vendlog($params, $error)
    {
      if (!isset($_SESSION['handle']))
        die("Not logged in");

      if (!is_numeric($params[0]) || (!is_numeric($params[1])))
      {
        $error->SetError(JsonRpcError_ParameterMismatch, "Non-numeric parameter(s)");
        return $error;
      }
  
      if (!check_permission($_SESSION['member_id'], "VIEW_VEND_LOG"))
      { 
        $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (VIEW_VEND_LOG)");
        return $error;          
      } 

      $link = db_link();
      $result = mysqli_query($link, "
        select
          vl.vend_tran_id,
          m.handle,
          vl.success_datetime,
          vl.cancelled_datetime, 
          concat('£', cast((vl.amount_scaled/100) as decimal(20,2))) as amount,
          coalesce(vr.loc_name, vl.position) as position,
          p.shortdesc as product,
          vl.denied_reason
        from vend_log vl
        left outer join members m on m.member_id = vl.member_id
        left outer join vmc_ref vr on vl.position = vr.loc_encoded
        left outer join transactions t on t.transaction_id = vl.transaction_id
        left outer join products p on p.product_id = t.product_id
        order by vl.vend_tran_id desc
        limit " . $params[0] . ", " . $params[1] . ";
      ");
      $rows = array();
      while($r = mysqli_fetch_assoc($result)) 
      {
        //$rows[] = array($row["vend_tran_id,"], $row["handle"], $row["success_datetime"], $row["cancelled_datetime"], $row["amount"], $row["position"], $row["denied_reason"]);
         $rows[] = $r;
      }
      mysqli_close($link);
      return json_encode($rows);    
    }


    function method_getbalances($params, $error)
    {
        if (!isset($_SESSION['handle']))
          die("Not logged in");
        
        if (!check_permission($_SESSION['member_id'], "VIEW_BALANCES"))
        { 
          $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (VIEW_BALANCES)");
          return $error;          
        }

        $link = db_link();
        $result = mysqli_query($link, "
        select 
          m.member_id,
          m.handle, 
          concat('£', cast((m.balance/100) as decimal(20,2))) as balance,
          concat('£', cast((m.credit_limit/100) as decimal(20,2))) as credit_limit,
          m.credit_limit as climit_int
        from members m
        order by m.handle;");
        $rows = array();
        while($row = mysqli_fetch_assoc($result)) 
        {
          $rows[] = array($row["member_id"], $row["handle"], $row["balance"], $row["credit_limit"], $row["climit_int"]);
        }
        mysqli_close($link);
        return json_encode($rows);
    }
      
    function method_gettransactions($params, $error)
    {
        if (!isset($_SESSION['handle']))
          die("Not logged in");
        
        if (!is_numeric($params[0]))
        {
          $error->SetError(JsonRpcError_ParameterMismatch, "Non-numeric parameter");
          return $error;
        }        
        
        // member id of -1 means return logged in users transactions
        if ($params[0] == -1)
          $params[0] = $_SESSION['member_id'];
        
        if ($params[0] == $_SESSION['member_id'])
        {
          if (!check_permission($_SESSION['member_id'], "VIEW_OWN_TRANS"))
          { 
            $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (VIEW_OWN_TRANS)");
            return $error;          
          }
        } else
        {
          if (!check_permission($_SESSION['member_id'], "VIEW_TRANS"))
          { 
            $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (VIEW_TRANS)");
            return $error;          
          }
        }
          
        $link = db_link();
        $result = mysqli_query($link, "
          select 
            t.transaction_datetime,
            t.transaction_status as status,
            t.transaction_type as type,
            concat('£', cast((t.amount/100) as decimal(20,2))) as amount,
            t.transaction_desc
          from transactions t
          inner join members m on m.member_id = t.member_id
          where m.member_id = $params[0]
          order by t.transaction_id desc;");
        $rows = array();
        while($row = mysqli_fetch_assoc($result)) 
        {
          $rows[] = array($row["transaction_datetime"], $row["status"], $row["type"], $row["amount"], $row["transaction_desc"]);
        }
        mysqli_close($link);
        return json_encode($rows);
    }
    
    
    function method_getgroups($params, $error)
    {
        if (!isset($_SESSION['handle']))
          die("Not logged in");    
        
        if (!check_permission($_SESSION['member_id'], "VIEW_GROUPS"))
        { 
          $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (VIEW_GROUPS)");
          return $error;          
        }

        $link = db_link();
        $result = mysqli_query($link, "
          select 
            g.grp_id,
            g.grp_description,
            (select count(*) from member_group mg where mg.grp_id = g.grp_id) as member_count
          from grp g
          order by grp_description;");
        $rows = array();
        while($row = mysqli_fetch_assoc($result)) 
        {
          $rows[] = array($row["grp_id"], $row["grp_description"], $row["member_count"]);
        }
        mysqli_close($link);
        return json_encode($rows);
    }
    
    function method_getaccessmemberlist($params, $error)
    {
        if (!isset($_SESSION['handle']))
          die("Not logged in");    
        
        if (!check_permission($_SESSION['member_id'], "VIEW_ACCESS_MEM"))
        { 
          $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (VIEW_ACCESS_MEM)");
          return $error;          
        }

        $link = db_link();
        $result = mysqli_query($link, "
          select 
            m.member_id,
            m.handle,
            case coalesce(ma.member_id,'-1')
              when '-1' then 'No'
              else 'Yes'
            end as pw_set,
            case (fn_check_permission(m.member_id, 'WEB_LOGON'))
              when 1 then 'Yes'
              else 'No'
            end as logon_en
          from members m
          left outer join members_auth ma on ma.member_id = m.member_id
          order by m.handle;");
        $rows = array();
        while($row = mysqli_fetch_assoc($result)) 
        {
          $rows[] = array($row["member_id"], $row["handle"], $row["pw_set"], $row["logon_en"]);
        }
        mysqli_close($link);
        return json_encode($rows);
    }    
    

    
    function method_changepassword($params, $error)
    {
        if (count($params) != 3)
        {
            $error->SetError(JsonRpcError_ParameterMismatch, "Expected 3 parameters; got " . count($params));
            return $error;
        }

        $link = db_link();
        $ret = $this->sp_web_change_password($link, $_SESSION['member_id'], $params[0], $params[1], $params[2]);
   
        mysqli_close($link);
        return $ret;
    }     
    
    function method_setpassword($params, $error)
    {
        if (count($params) != 3)
        {
            $error->SetError(JsonRpcError_ParameterMismatch, "Expected 3 parameters; got " . count($params));
            return $error;
        }

        if (!check_permission($_SESSION['member_id'], "SET_PASSWORD"))
        { 
          $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (SET_PASSWORD)");
          return $error;          
        }        
        
        
        $link = db_link();
        $ret = $this->sp_web_set_password($link, $params[0], $params[1], $params[2]);
   
        mysqli_close($link);
        return $ret;
    } 
       
    
    function method_setclimit($params, $error)
    {
        if (!isset($_SESSION['handle']))
          die("Not logged in");
        
        if (!check_permission($_SESSION['member_id'], "SET_CREDIT_LIMIT"))
        { 
          $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (SET_CREDIT_LIMIT)");
          return $error;          
        }        
        
        if ((!is_numeric($params[0])) || (!is_numeric($params[1])))
        {
          $error->SetError(JsonRpcError_ParameterMismatch, "Non-numeric parameter");
          return $error;
        }   

        $link = db_link();
        $ret = $this->sp_set_credit_limit($link, $params[0], $params[1]);
   
        mysqli_close($link);
        return $ret;
    }        
        

    /*
      param[0] = amount
      param[1] = description
      param[2] = member_id to update 
    */
    function method_recordtran($params, $error)
    {
        if (!isset($_SESSION['handle']))
          die("Not logged in");
        
        if ($params[2] == -1)
          $params[2] = $_SESSION['member_id'];
        
        if ($params[2] == $_SESSION['member_id'])
        {
          if (!check_permission($_SESSION['member_id'], "REC_TRAN_OWN"))
          { 
            $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (REC_TRAN_OWN)");
            return $error;          
          }
        } else
        {
          if (!check_permission($_SESSION['member_id'], "REC_TRAN"))
          { 
            $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (REC_TRAN)");
            return $error;          
          }
        }

        $link = db_link();
        $ret = $this->sp_transaction_log($link, $params[2], $params[0], "MANUAL", "COMPLETE", $params[1], $_SESSION['member_id']);
   
        mysqli_close($link);
        return $ret;  
    }
    
    function method_removegroupmember($params, $error)
    {
        if (!isset($_SESSION['handle']))
          die("Not logged in");
        
        if (!check_permission($_SESSION['member_id'], "REM_GRP_MEMBER"))
        { 
          $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (REM_GRP_MEMBER)");
          return $error;          
        }

        $link = db_link();
        $ret = $this->sp_web_group_delete_member($link, $params[0], $params[1]);
   
        mysqli_close($link);
        return $ret;  
    }    
    
    function method_addgroupmember($params, $error)
    {
        if (!isset($_SESSION['handle']))
          die("Not logged in");
        
        if (!check_permission($_SESSION['member_id'], "ADD_GRP_MEMBER"))
        { 
          $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (ADD_GRP_MEMBER)");
          return $error;          
        }

        $link = db_link();
        $ret = $this->sp_web_group_add_member($link, $params[0], $params[1]);
   
        mysqli_close($link);
        return $ret;  
    }      
    
    function method_togglegrouppermission($params, $error)
    {
        if (!isset($_SESSION['handle']))
          die("Not logged in");
        
        if (!check_permission($_SESSION['member_id'], "CHG_GRP_PERM"))
        { 
          $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (CHG_GRP_PERM)");
          return $error;          
        }

        $link = db_link();
        $ret = $this->sp_web_group_toggle_permission($link, $params[0], $params[1]);
   
        mysqli_close($link);
        return $ret;  
    }      
    
    function method_addgroup($params, $error)
    {
        if (!isset($_SESSION['handle']))
          die("Not logged in");
        
        if (!check_permission($_SESSION['member_id'], "ADD_GROUP"))
        { 
          $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (ADD_GROUP)");
          return $error;          
        }

        $link = db_link();
        $ret = $this->sp_web_group_add($link, $params[0]);
   
        mysqli_close($link);
        return $ret;  
    }        
    
    function method_deletegroup($params, $error)
    {
        if (!isset($_SESSION['handle']))
          die("Not logged in");
        
        if (!check_permission($_SESSION['member_id'], "DEL_GROUP"))
        { 
          $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (DEL_GROUP)");
          return $error;          
        }

        $link = db_link();
        $ret = $this->sp_web_group_del($link, $params[0]);
   
        mysqli_close($link);
        return $ret;  
    }     
    
    
    
    
    function method_getgroupmembers($params, $error)
    {
      
      if (!isset($_SESSION['handle']))
        die("Not logged in");
        
      if (!check_permission($_SESSION['member_id'], "VIEW_GRP_MEMBERS"))
      { 
        $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (VIEW_GRP_MEMBERS)");
        return $error;          
      }        
        
      $link = db_link();
      if ($stmt = mysqli_prepare($link, "
        select 
          m.member_id,
          m.handle,
          m.member_number
        from members m
        inner join member_group mg on mg.member_id = m.member_id
        where mg.grp_id = ?")) 
      {
        mysqli_stmt_bind_param($stmt, "i", $params[0]);
        if (mysqli_stmt_execute($stmt))
        {
          mysqli_stmt_bind_result($stmt, $member_id, $handle, $member_number);
          $rows = array();
          while (mysqli_stmt_fetch($stmt))
          {
            $rows[] = array($member_id, $handle, $member_number);
          }
          mysqli_stmt_close($stmt); 
        } else
        {
          mysqli_stmt_close($stmt);    
          mysqli_close($link);
          return "ERR1";          
        }
      } else
      {
        mysqli_close($link);
        return "ERR2";
      }
      
      mysqli_close($link);
      return json_encode($rows); 
    }     
    
    function method_getgrouppermissions($params, $error)
    {
      
      if (!isset($_SESSION['handle']))
        die("Not logged in");
        
      if (!check_permission($_SESSION['member_id'], "VIEW_GRP_PERMIS"))
      { 
        $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (VIEW_GRP_PERMIS)");
        return $error;          
      }        
        
      $link = db_link();
      if ($stmt = mysqli_prepare($link, "
        select
          p.permission_code,
          p.permission_desc,
          case coalesce(gp.permission_code,'NO')
            when 'No' then 'Disabled'
            else 'Enabled'
          end as state
        from permissions p
        left outer join group_permissions gp on gp.permission_code = p.permission_code and gp.grp_id = ?
        order by p.permission_code;")) 
      {
        mysqli_stmt_bind_param($stmt, "i", $params[0]);
        if (mysqli_stmt_execute($stmt))
        {
          mysqli_stmt_bind_result($stmt, $permission_code, $permission_desc, $state);
          $rows = array();
          while (mysqli_stmt_fetch($stmt))
          {
            $rows[] = array($permission_code, $permission_desc, $state);
          }
          mysqli_stmt_close($stmt); 
        } else
        {
          mysqli_stmt_close($stmt);    
          mysqli_close($link);
          return "ERR1";          
        }
      } else
      {
        mysqli_close($link);
        return "ERR2";
      }
      
      mysqli_close($link);
      return json_encode($rows); 
    } 
    
    
    function method_getpermissions($params, $error)
    {
      
      if (!isset($_SESSION['handle']))
        die("Not logged in");       
        
      $link = db_link();
      if ($stmt = mysqli_prepare($link, "
        select distinct rtrim(gp.permission_code) as permission_code
        from member_group mg
        inner join group_permissions gp on mg.grp_id = gp.grp_id
        where mg.member_id = ?")) 
      {
        mysqli_stmt_bind_param($stmt, "i", $_SESSION['member_id']);
        if (mysqli_stmt_execute($stmt))
        {
          mysqli_stmt_bind_result($stmt, $permission_code);
          $rows = array();
          while (mysqli_stmt_fetch($stmt))
          {
            $rows[] = array($permission_code);
          }
          mysqli_stmt_close($stmt); 
        } else
        {
          mysqli_stmt_close($stmt);    
          mysqli_close($link);
          return "ERR1";          
        }
      } else
      {
        mysqli_close($link);
        return "ERR2";
      }
      
      mysqli_close($link);
      return json_encode($rows); 
    } 
        
    
    
    function method_memberlistbox($params, $error)
    {
      if (!isset($_SESSION['handle']))
        die("Not logged in");
        
      if (!check_permission($_SESSION['member_id'], "VIEW_MEMBERS"))
      { 
        $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (VIEW_MEMBERS)");
        return $error;          
      }  

        $link = db_link();
        $result = mysqli_query($link, "
          select 
            m.member_id,
            m.handle 
          from members m
          order by m.handle;");
        $rows = array();
        while($row = mysqli_fetch_assoc($result)) 
        {
          $rows[] = array($row["member_id"], $row["handle"]);
        }
        mysqli_close($link);
        return json_encode($rows);
    }
    

    function method_getsales($params, $error)
    {
      
      if (!isset($_SESSION['handle']))
        die("Not logged in");
        
      if (!check_permission($_SESSION['member_id'], "VIEW_SALES"))
      { 
        $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (VIEW_SALES)");
        return $error;          
      }        
        
      $link = db_link();
      if ($stmt = mysqli_prepare($link, "
        select 
          t.transaction_id,
          t.transaction_datetime,
          m.handle,
          t.transaction_type,
          concat('£', cast((-1*t.amount/100) as decimal(20,2))) as price
        from transactions t 
        inner join members m on t.member_id = m.member_id
        where t.product_id = ?
        order by t.transaction_datetime desc;"
        )) 
      {
        mysqli_stmt_bind_param($stmt, "i", $params[0]);
        if (mysqli_stmt_execute($stmt))
        {
          mysqli_stmt_bind_result($stmt, $transaction_id, $transaction_datetime, $handle, $transaction_type, $price);
          $rows = array();
          while (mysqli_stmt_fetch($stmt))
          {
            $rows[] = array($transaction_id, $transaction_datetime, $handle, $transaction_type, $price);
          }
          mysqli_stmt_close($stmt); 
        } else
        {
          mysqli_stmt_close($stmt);    
          mysqli_close($link);
          return "ERR1";          
        }
      } else
      {
        mysqli_close($link);
        return "ERR2";
      }
      
      mysqli_close($link);
      return json_encode($rows); 
    }         
           
    function method_gettransactionstatus($params, $error)
    {
      $status_str = "Error";
      if (!isset($_SESSION['handle']))
        die("Not logged in");
        
        // member id of -1 means return logged in users transactions
        if ($params[0] == -1)
          $params[0] = $_SESSION['member_id'];
        
        if ($params[0] == $_SESSION['member_id'])
        { // This function is only called from transactions screen, so it seems to make sense
          // that it shares the same permissions code at this point.
          if (!check_permission($_SESSION['member_id'], "VIEW_OWN_TRANS"))
          { 
            $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (VIEW_OWN_TRANS)");
            return $error;          
          }
        } else
        {
          if (!check_permission($_SESSION['member_id'], "VIEW_TRANS"))
          { 
            $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (VIEW_TRANS)");
            return $error;          
          }
        }     
        
      $link = db_link();
      if ($stmt = mysqli_prepare($link, "
        select 
          concat('£', cast((m.credit_limit/100) as decimal(20,2))) as credit_limit,
          concat('£', cast((m.balance/100) as decimal(20,2))) as balance
        from members m
        where m.member_id = ?")) 
      {
        mysqli_stmt_bind_param($stmt, "i", $params[0]);
        if (mysqli_stmt_execute($stmt))
        {
          mysqli_stmt_bind_result($stmt, $credit_limit, $balance);
          $rows = array();
          mysqli_stmt_fetch($stmt);
          
          $status_str = "Balance = $balance, Credit limit = $credit_limit";
          
          mysqli_stmt_close($stmt); 
        } else
        {
          mysqli_stmt_close($stmt);    
          mysqli_close($link);
          return "ERR1";          
        }
      } else
      {
        mysqli_close($link);
        return "ERR2";
      }
      
      mysqli_close($link);
      return $status_str;
    }     
       
       
       
       
           
    
    function method_logoff($params, $error)
    {
      setcookie (session_id(), "", time() - 3600);
      session_destroy();
      session_regenerate_id (true);
      session_write_close();
    }

      



   

    // *********************************************************************************************************
    
    function sp_set_credit_limit($connection, $member_id, $credit_limit)
    {
      
      if ($stmt = mysqli_prepare($connection, "call sp_set_credit_limit(?, ?, @err)")) 
      {
        mysqli_stmt_bind_param($stmt, "ii", $member_id, $credit_limit);
        if (!mysqli_stmt_execute($stmt))
        {
          mysqli_stmt_close($stmt);   
          return "ERR";
        }
        mysqli_stmt_close($stmt);    
      } else
      {
        return "ERR";
      }

      // Get result
      if ($stmt = mysqli_prepare($connection, "select @err as err")) 
      {
        mysqli_stmt_execute($stmt);
        mysqli_stmt_bind_result($stmt, $err);
        mysqli_stmt_fetch($stmt);
        mysqli_stmt_close($stmt);    
      }
      
      return $err;
    }    

    function sp_transaction_log($connection, $member_id, $amount, $tran_type, $tran_status, $tran_desc, $recorded_by)
    {
      
      if ($stmt = mysqli_prepare($connection, "call sp_transaction_log(?, ?, ?, ?, ?, ?, @trnid, @err)")) 
      {
        mysqli_stmt_bind_param($stmt, "iisssi", $member_id, $amount, $tran_type, $tran_status, $tran_desc, $recorded_by);
        if (!mysqli_stmt_execute($stmt))
        {
          mysqli_stmt_close($stmt);   
          return "ERR";
        }
        mysqli_stmt_close($stmt);    
      } else
      {
        return "ERR";
      }

      // Get result
      if ($stmt = mysqli_prepare($connection, "select @err as err, @trnid as trnid")) 
      {
        mysqli_stmt_execute($stmt);
        mysqli_stmt_bind_result($stmt, $err, $trnid);
        mysqli_stmt_fetch($stmt);
        mysqli_stmt_close($stmt);    
      }
      
      return $err;
    }        
    
    
    function sp_web_prelogin($connection, $handle)
    {

      if (strlen($handle) > 100)
      {
        return "";
      }
      
      if ($stmt = mysqli_prepare($connection, "call sp_web_prelogin(?, @salt)")) 
      {
        mysqli_stmt_bind_param($stmt, "s", $handle);
        if (!mysqli_stmt_execute($stmt))
        {
          mysqli_stmt_close($stmt);   
          return "";
        }
        mysqli_stmt_close($stmt);    
      } else
      {
        return "";
      }

      // Get result
      if ($stmt = mysqli_prepare($connection, "select @salt as salt")) 
      {
        mysqli_stmt_execute($stmt);
        mysqli_stmt_bind_result($stmt, $salt);
        mysqli_stmt_fetch($stmt);
        mysqli_stmt_close($stmt);    
          
        if (is_null($salt))
        {
          return "";
        }
      } else
      {
        return "";
      }
      
      return $salt;
    }

    // Nb: $password is already hashed with salt (done in javascript)
    function sp_web_login($connection, $handle, $password, &$member_id)
    {

      if ((strlen($handle) > 100) || (strlen($password) != 40))
      {
        return false;
      }
      
      if ($stmt = mysqli_prepare($connection, "call sp_web_login(?, ?, @ret, @member_id)")) 
      {
        mysqli_stmt_bind_param($stmt, "ss", $handle, $password);
        if (!mysqli_stmt_execute($stmt))
        {
          mysqli_stmt_close($stmt);   
          return false;
        }
        mysqli_stmt_close($stmt);    
      } else
      {
        return false;
      }

      // Get result
      if ($stmt = mysqli_prepare($connection, "select @ret as ret, @member_id as member_id")) 
      {
        mysqli_stmt_execute($stmt);
        mysqli_stmt_bind_result($stmt, $ret, $member_id);
        mysqli_stmt_fetch($stmt);
        mysqli_stmt_close($stmt);    
          
        if (is_null($ret))
        {
          return false; 
        }
      } else
      {
        return false;
      }

      if ($ret == 1)
        return true;
      else
        return false;
    }

    function sp_web_set_vendprd($connection, $member_id, $loc_name, $product_id)
    {
      
      if ($stmt = mysqli_prepare($connection, "call sp_web_set_vendprd(?, ?, ?, @err)")) 
      {
        mysqli_stmt_bind_param($stmt, "isi", $member_id, $loc_name, $product_id);
        if (!mysqli_stmt_execute($stmt))
        {
          mysqli_stmt_close($stmt);   
          return "ERR";
        }
        mysqli_stmt_close($stmt);    
      } else
      {
        return "ERR";
      }

      // Get result
      if ($stmt = mysqli_prepare($connection, "select @err as err")) 
      {
        mysqli_stmt_execute($stmt);
        mysqli_stmt_bind_result($stmt, $err);
        mysqli_stmt_fetch($stmt);
        mysqli_stmt_close($stmt);    
      }
      
      return $err;
    }    

    function sp_web_updateproduct($connection, $member_id, $product_id, $price, $barcode, $available, $shortdesc, $longdesc)
    {
      
      if ($stmt = mysqli_prepare($connection, "call sp_web_updateproduct(?, ?, ?, ?, ?, ?, ?, @err)")) 
      {
        mysqli_stmt_bind_param($stmt, "iiisiss", $member_id, $product_id, $price, $barcode, $available, $shortdesc, $longdesc);
        if (!mysqli_stmt_execute($stmt))
        {
          mysqli_stmt_close($stmt);   
          return "ERR1";
        }
        mysqli_stmt_close($stmt);    
      } else
      {
        return "ERR2";
      }

      // Get result
      if ($stmt = mysqli_prepare($connection, "select @err as err")) 
      {
        mysqli_stmt_execute($stmt);
        mysqli_stmt_bind_result($stmt, $err);
        mysqli_stmt_fetch($stmt);
        mysqli_stmt_close($stmt);    
      }
      
      return $err;
    }    
    
    
    
    
    function sp_web_change_password($connection, $member_id, $curPass, $new_salt, $new_password)
    {
      
      if ($stmt = mysqli_prepare($connection, "call sp_web_change_password(?, ?, ?, ?, @err);")) 
      {
        mysqli_stmt_bind_param($stmt, "isss", $member_id, $curPass, $new_salt, $new_password);
        if (!mysqli_stmt_execute($stmt))
        {
          mysqli_stmt_close($stmt);   
          return "ERR1";;
        }
        mysqli_stmt_close($stmt);    
      } else
      {
        return "ERR2";
      }

      // Get result
      if ($stmt = mysqli_prepare($connection, "select @err as err")) 
      {
        mysqli_stmt_execute($stmt);
        mysqli_stmt_bind_result($stmt, $err);
        mysqli_stmt_fetch($stmt);
        mysqli_stmt_close($stmt);    
      }
      
      return $err;
    }   
        
    function sp_web_set_password($connection, $member_id, $new_salt, $new_password)
    {
      
      if ($stmt = mysqli_prepare($connection, "call sp_web_set_password(?, ?, ?, @err);")) 
      {
        mysqli_stmt_bind_param($stmt, "iss", $member_id, $new_salt, $new_password);
        if (!mysqli_stmt_execute($stmt))
        {
          mysqli_stmt_close($stmt);   
          return "ERR1";;
        }
        mysqli_stmt_close($stmt);    
      } else
      {
        return "ERR2";
      }

      // Get result
      if ($stmt = mysqli_prepare($connection, "select @err as err")) 
      {
        mysqli_stmt_execute($stmt);
        mysqli_stmt_bind_result($stmt, $err);
        mysqli_stmt_fetch($stmt);
        mysqli_stmt_close($stmt);    
      }
      
      return $err;
    }   
    
    
    function sp_web_group_delete_member($connection, $grp_id, $member_id)
    {
      
      if ($stmt = mysqli_prepare($connection, "call sp_web_group_delete_member(?, ?, @err);")) 
      {
        mysqli_stmt_bind_param($stmt, "ii", $grp_id, $member_id);
        if (!mysqli_stmt_execute($stmt))
        {
          mysqli_stmt_close($stmt);   
          return "ERR1";;
        }
        mysqli_stmt_close($stmt);    
      } else
      {
        return "ERR2";
      }

      // Get result
      if ($stmt = mysqli_prepare($connection, "select @err as err")) 
      {
        mysqli_stmt_execute($stmt);
        mysqli_stmt_bind_result($stmt, $err);
        mysqli_stmt_fetch($stmt);
        mysqli_stmt_close($stmt);    
      }
      
      return $err;
    }       
    
    function sp_web_group_add_member($connection, $grp_id, $member_id)
    {
      
      if ($stmt = mysqli_prepare($connection, "call sp_web_group_add_member(?, ?, @err);")) 
      {
        mysqli_stmt_bind_param($stmt, "ii", $grp_id, $member_id);
        if (!mysqli_stmt_execute($stmt))
        {
          mysqli_stmt_close($stmt);   
          return "ERR1";
        }
        mysqli_stmt_close($stmt);    
      } else
      {
        return "ERR2";
      }

      // Get result
      if ($stmt = mysqli_prepare($connection, "select @err as err")) 
      {
        mysqli_stmt_execute($stmt);
        mysqli_stmt_bind_result($stmt, $err);
        mysqli_stmt_fetch($stmt);
        mysqli_stmt_close($stmt);    
      }
      
      return $err;
    }   
    
    function sp_web_group_toggle_permission($connection, $grp_id, $permission_code)
    {
      
      if ($stmt = mysqli_prepare($connection, "call sp_web_group_toggle_permission(?, ?, @err);")) 
      {
        mysqli_stmt_bind_param($stmt, "is", $grp_id, $permission_code);
        if (!mysqli_stmt_execute($stmt))
        {
          mysqli_stmt_close($stmt);   
          return "ERR1";
        }
        mysqli_stmt_close($stmt);    
      } else
      {
        return "ERR2";
      }

      // Get result
      if ($stmt = mysqli_prepare($connection, "select @err as err")) 
      {
        mysqli_stmt_execute($stmt);
        mysqli_stmt_bind_result($stmt, $err);
        mysqli_stmt_fetch($stmt);
        mysqli_stmt_close($stmt);    
      }
      
      return $err;
    }       
    
    function sp_web_group_add($connection, $grp_description)
    {
      
      if ($stmt = mysqli_prepare($connection, "call sp_web_group_add(?, @err);")) 
      {
        mysqli_stmt_bind_param($stmt, "s", $grp_description);
        if (!mysqli_stmt_execute($stmt))
        {
          mysqli_stmt_close($stmt);   
          return "ERR1";
        }
        mysqli_stmt_close($stmt);    
      } else
      {
        return "ERR2";
      }

      // Get result
      if ($stmt = mysqli_prepare($connection, "select @err as err")) 
      {
        mysqli_stmt_execute($stmt);
        mysqli_stmt_bind_result($stmt, $err);
        mysqli_stmt_fetch($stmt);
        mysqli_stmt_close($stmt);    
      }
      
      return $err;
    }       
    
    function sp_web_group_del($connection, $grp_id)
    {
      
      if ($stmt = mysqli_prepare($connection, "call sp_web_group_del(?, @err);")) 
      {
        mysqli_stmt_bind_param($stmt, "i", $grp_id);
        if (!mysqli_stmt_execute($stmt))
        {
          mysqli_stmt_close($stmt);   
          return "ERR1";
        }
        mysqli_stmt_close($stmt);    
      } else
      {
        return "ERR2";
      }

      // Get result
      if ($stmt = mysqli_prepare($connection, "select @err as err")) 
      {
        mysqli_stmt_execute($stmt);
        mysqli_stmt_bind_result($stmt, $err);
        mysqli_stmt_fetch($stmt);
        mysqli_stmt_close($stmt);    
      }
      
      return $err;
    }       
    
}



?>  
