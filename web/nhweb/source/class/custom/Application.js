/* ************************************************************************

   Copyright:

   License:

   Authors:

************************************************************************ */

/* ************************************************************************

#asset(custom/*)

************************************************************************ */

/**
 * This is the main application class of your custom application "custom"
 */
qx.Class.define("custom.Application",
{
  extend : qx.application.Standalone,



  /*
  *****************************************************************************
     MEMBERS
  *****************************************************************************
  */

  members :
  {
    gURL : "http://192.168.1.148/web/rpcservice/services/",
    gSalt : "",
    gPermissions : qx.data.Array,           
  //gURL : "http://hollyvm/nh-web/rpc/services/",
    /**
     * This method contains the initial application code and gets called 
     * during startup of the application
     * 
     * @lint ignoreDeprecated(alert)
     */
    main : function()
    {
      // Call super class
      this.base(arguments);

      // Enable logging in debug variant
      if (qx.core.Environment.get("qx.debug"))
      {
        // support native logging capabilities, e.g. Firebug for Firefox
        qx.log.appender.Native;
        // support additional cross-browser console. Press F7 to toggle visibility
        qx.log.appender.Console;
      }

      /*
      -------------------------------------------------------------------------
        Below is your actual application code...
      -------------------------------------------------------------------------
      */

      // Create a button
    //  var button1 = new qx.ui.form.Button("First Button", "custom/test.png");

      // Document is the application root
      var doc = this.getRoot();
/*
      // Add button to document at fixed coordinates
     // doc.add(button1, {left: 100, top: 50});


  */

    var wlog = new custom.wLogon();
    wlog.open();
    wlog.addListener("resize", function()
    {
      this.center();
    }, wlog);

    wlog.addListener("login", function(e) 
    {
      // get permissions
      var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");
      try 
      {
        var ret = rpc.callSync("getpermissions");
      } catch (exc) 
      {
          alert("Exception during sync call: " + exc);
          return false;
      }      
      gPermissions = new qx.data.Array(qx.lang.Json.parse(ret));
        
      // Create menu bar  
      var bar = this.getMenuBar();
      doc.add(bar, {left: 0, top: 0});      
      }, this);
    

    },
                
    gotPermission : function (permission)
    {
      for (n = 0; n < gPermissions.length; n++)
      {
        if (gPermissions.getItem(n).toString() == permission)
          return true;
      }
      return false;
    },
                
    getMenuBar : function()
    {
      var snackSpaceMenu;
      var accountMenu;
      var accessMenu;
      
      var menubar = new qx.ui.menubar.MenuBar;
      menubar.setWidth(600);

       
      snackSpaceMenu = this.getSnackMenu();
      if (snackSpaceMenu.getChildren().length > 0)
      {
        var snackMenuB = new qx.ui.menubar.Button("Snackspace", null, snackSpaceMenu);    
        menubar.add(snackMenuB);
      }
      
      accountMenu = this.getAccountMenu();
      if (accountMenu.getChildren().length > 0)
      {
        var accountMenuB = new qx.ui.menubar.Button("Account", null, accountMenu);    
        menubar.add(accountMenuB);     
      }
      
      accessMenu = this.getAccessMenu();
      if (accessMenu.getChildren().length > 0)
      {
        var accessMenuB = new qx.ui.menubar.Button("Access", null, accessMenu);    
        menubar.add(accessMenuB);        
      }

      return menubar;
    },
  
    getSnackMenu : function()
    {
      var menu = new qx.ui.menu.Menu;
      
      //Vend config
      if (this.gotPermission("VIEW_VEND_CONFIG"))
      {
        var vendButton = new qx.ui.menu.Button("Vend setup", null);
        vendButton.addListener("execute", function(e) 
        {
          var main = new custom.wVendConfig();
          main.moveTo(50, 30);
          main.open();
        });
        menu.add(vendButton);
      }

      //Products
      if (this.gotPermission("VIEW_PRODUCTS"))
      {
        var productButton = new qx.ui.menu.Button("Products", null);
        productButton.addListener("execute", function(e) 
        {
          var main = new custom.wProducts();
          main.moveTo(50, 30);
          main.open();
        });
        menu.add(productButton);
      }

      //Vend Log
      if (this.gotPermission("VIEW_VEND_LOG"))
      {
        var vendLogButton = new qx.ui.menu.Button("Vend Log", null);
        vendLogButton.addListener("execute", function(e) 
        {
          var main = new custom.wVendLog();
          main.moveTo(50, 30);
          main.open();
        });
        menu.add(vendLogButton);     
      }

      // Balances
      if (this.gotPermission("VIEW_BALANCES"))
      {
        var balButton = new qx.ui.menu.Button("Balances", null);
        balButton.addListener("execute", function(e) 
        {
          var main = new custom.wBalances();
          main.moveTo(50, 30);
          main.open();
        });
        menu.add(balButton);      
      }
      
      return menu;
    },
                
    getAccessMenu : function()
    {
      var menu = new qx.ui.menu.Menu;
      
      //Groups
      if (this.gotPermission("VIEW_GROUPS"))
      {
        var groupButton = new qx.ui.menu.Button("Groups", null);
        groupButton.addListener("execute", function(e) 
        {
          var main = new custom.wGroups();
          main.moveTo(50, 30);
          main.open();
        });
        menu.add(groupButton);
      }

     
      //Members
      if (this.gotPermission("VIEW_ACCESS_MEM"))
      {      
        var memberButton = new qx.ui.menu.Button("Members", null);
        memberButton.addListener("execute", function(e) 
        {
          var main = new custom.wMemberAccess();
          main.moveTo(50, 30);
          main.open();
        });
        menu.add(memberButton);   
      }
      
      return menu;
    },            
                
    getAccountMenu : function()
    {
      var menu = new qx.ui.menu.Menu;
      
      // Transactions (own)
      if (this.gotPermission("VIEW_OWN_TRANS"))
      {      
        var tranButton = new qx.ui.menu.Button("Transactions", null);
        tranButton.addListener("execute", function(e) 
        {
          var memtrn = new custom.wTransactions(-1, "");
          memtrn.moveTo(55, 35);
        //memtrn.setModal(true); 
          memtrn.open();
        });
        menu.add(tranButton);       
      }
      
      // log off button
      var logoffButton = new qx.ui.menu.Button("Logout", null);
      logoffButton.addListener("execute", function(e) 
      {
        var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");
      
        try 
        {
          rpc.callSync("logoff");
        } catch (exc) 
        {
            alert("Exception during sync call: " + exc);
            return false;
        }
        window.top.location.reload(); 
      });
      menu.add(logoffButton);      

    // Change password
      var chpasButton = new qx.ui.menu.Button("Change Password", null);
      chpasButton.addListener("execute", function(e) 
      {
        var main = new custom.wChangePassword();
        main.moveTo(50, 30);
        main.open();
      });
      menu.add(chpasButton);       
      
      
      return menu;
    }
    

    

  }

 
  
});
