qx.Class.define("custom.wChangePassword",
{
  extend : qx.ui.window.Window,

  construct : function(member_id)
  {
    var chgPass; // chgPass = true means changing own password, false means setting someones
    
    if (typeof member_id == 'undefined') 
      chgPass = true;
    else
      chgPass = false;
    

    this.base(arguments, "Change password");

    this.setShowClose(true);
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
    this.add(new qx.ui.basic.Label("Current password"), {row: 0, column: 0});
    this.add(new qx.ui.basic.Label("New password") ,    {row: 1, column: 0}); 
    this.add(new qx.ui.basic.Label("Repeat password"),  {row: 2, column: 0}); 
    
    // Details entry
    var txtCurPass  = new qx.ui.form.PasswordField();
    var txtNewPass1 = new qx.ui.form.PasswordField();
    var txtNewPass2 = new qx.ui.form.PasswordField();
    
    this.add(txtCurPass,  {row: 0, column: 1});
    this.add(txtNewPass1, {row: 1, column: 1});
    this.add(txtNewPass2, {row: 2, column: 1});
    
    txtCurPass.setEnabled(chgPass);    
    
    var modelSkeleton = {CurPass: null, NewPass1: null, NewPass2: null};
    var model = qx.data.marshal.Json.createModel(modelSkeleton);

    // create the controller
    var controller = new qx.data.controller.Object(model);
  
    // connect the name
    controller.addTarget(txtCurPass,  "value", "CurPass" , true);   
    controller.addTarget(txtNewPass1, "value", "NewPass1", true);
    controller.addTarget(txtNewPass2, "value", "NewPass2", true);

    txtCurPass.setRequired(chgPass);
    txtNewPass1.setRequired(true);
    txtNewPass2.setRequired(true);

    // create the manager
    var manager = new qx.ui.form.validation.Manager();
    manager.add(txtCurPass);
    manager.add(txtNewPass1);
    manager.add(txtNewPass2);

    // Change button
    var chgButton = new qx.ui.form.Button("Change");
    this.add(chgButton, {row: 3, column: 0, colSpan: 2});
    chgButton.setToolTipText("Change Password");

    chgButton.addListener("execute", function() 
    {
      if (manager.validate())
      {       
        this.changePassword(txtCurPass.getValue(), txtNewPass1.getValue(), txtNewPass2.getValue(), chgPass, member_id);
        this.close();
      }
    }, this); 
   
    chgButton.setEnabled(true); 
  },

  members:
  {

    changePassword : function(curPass, newPass1, newPass2, chgPass, member_id)
    {
      
      // First, check two new passwords match
      if (newPass1 != newPass2)
      {
          alert ("New passwords don't match!");
          return;
      }

      var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");

      try 
      {
        if (chgPass)
          var res = rpc.callSync("changepassword", curPass, newPass1);
        else
          var res = rpc.callSync("setpassword", member_id, newPass1);
        
        if (res != "")
          alert("Failed: " + res);
      } catch (exc) 
      {
          alert("Exception during sync call: " + exc);

      }

      return;
    }
  }
});

