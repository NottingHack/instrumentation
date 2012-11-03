qx.Class.define("custom.wLogon",
{
  extend : qx.ui.window.Window,

  construct : function()
  {
    this.base(arguments, "Logon");

    // hide the window buttons
    this.setShowClose(false);
    this.setShowMaximize(false);
    this.setShowMinimize(false);

    // adjust size
   // this.setWidth(250);
   // this.setHeight(300);

    // add the layout
    var layout = new qx.ui.layout.Grid(0, 0);
    layout.setRowFlex(1, 1);
    layout.setColumnFlex(1, 1);
    this.setLayout(layout);
    this.setContentPadding(0);
 
    // add labels
    this.add(new qx.ui.basic.Label("Handle: "), {row: 0, column: 0});
    this.add(new qx.ui.basic.Label("Password: "), {row: 1, column: 0}); 

    // Handle entry
    var txtHandle = new qx.ui.form.TextField();
    this.add(txtHandle, {row: 0, column: 1});

    // Password entry
    var txtPass = new qx.ui.form.PasswordField();
    this.add(txtPass, {row: 1, column: 1});    
      
    var modelSkeleton = {username: null, password: null};
    var model = qx.data.marshal.Json.createModel(modelSkeleton);

    // create the controller
    var controller = new qx.data.controller.Object(model);

    // connect the name
    controller.addTarget(txtHandle, "value", "username", true);
    controller.addTarget(txtPass, "value", "password", true);
    txtHandle.setRequired(true);
    txtPass.setRequired(true);

    // create the manager
    var manager = new qx.ui.form.validation.Manager();
    manager.add(txtHandle);
    manager.add(txtPass);


   
    // post button
    var postButton = new qx.ui.form.Button("Login");
    this.add(postButton, {row: 2, column: 0, colSpan: 2});
    postButton.setToolTipText("Login");

    postButton.addListener("execute", function() 
    {
      if (manager.validate())
      {
        if (this.doLogon(txtHandle.getValue(), txtPass.getValue()))
        {
          this.fireEvent("login");
          this.close();
        } else
        {
          alert ("Invalid login...");
        }
      }
    }, this); 
    postButton.setWidth(60);
    postButton.setEnabled(true);
  },

  members:
  {

    doLogon : function(username, password)
    {
      var rpc = new qx.io.remote.Rpc(
          qx.core.Init.getApplication().gURL,
          "qooxdoo.nhweb"
      );
 
      // do login
      result = false;
      try 
      {
        var result = rpc.callSync("login", username, password);
      } catch (exc) 
      {
          alert("Exception during sync call: " + exc);
          return false;
      }

      if (result == true)
      {
        return true;
      }
      else
        return false;
    }
  },


  events :
  {
    "login" : "qx.event.type.Event"
  }
});

