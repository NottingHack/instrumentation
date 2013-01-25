qx.Class.define("custom.wPINUpdate",
{
  extend : qx.ui.window.Window,

  construct : function(member_id, pin_id, pin_val, pin_expiry, pin_state)
  {
    if (pin_id == -1) 
      this.base(arguments, "Add PIN");
    else
      this.base(arguments, "Amend PIN");

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
    this.add(new qx.ui.basic.Label("PIN: ")    , {row: 0, column: 0});
    this.add(new qx.ui.basic.Label("Expiry: ") , {row: 1, column: 0}); 
    this.add(new qx.ui.basic.Label("State: ")  , {row: 2, column: 0}); 



    // Details entry
    var fPIN    = new qx.ui.form.TextField();
    var fExpiry = new qx.ui.form.DateField();
    var fState  = new qx.ui.form.SelectBox();

    fPIN.setFilter(/[0-9]/);
    
    // Combo box options
        
    var comboBoxList = [{name: "Active", id: 10},{name: "Expired", id: 20},{name: "Cancelled", id: 30},{name: "Enroll", id: 40}]; 
    var state_model = qx.data.marshal.Json.createModel(comboBoxList); 
    var state_controller = new qx.data.controller.List(state_model, fState, "name"); 
       
    
    this.add(fPIN   , {row: 0, column: 1});
    this.add(fExpiry, {row: 1, column: 1});
    this.add(fState , {row: 2, column: 1});
    
       
    var modelSkeleton = {PIN: null, Expiry: null, State: null};
    var model = qx.data.marshal.Json.createModel(modelSkeleton);

    // create the controller
    var controller = new qx.data.controller.Object(model);

    // connect the name
    controller.addTarget(fPIN   , "value", "PIN"   , true);   
    controller.addTarget(fExpiry, "value", "Expiry", true);
  //controller.addTarget(fState , "value", "State" , true); 
   
    if (pin_id != -1)
    {
      fPIN.setRequired(false);
      fPIN.setEnabled(false);
    } else
    {
      fPIN.setRequired(true);
    }
    fExpiry.setRequired(false);
    fState.setRequired(true);

    // create the manager
    var manager = new qx.ui.form.validation.Manager();
    manager.add(fPIN);
    manager.add(fExpiry);
  //manager.add(fState);

    // Add button
    var addButton = new qx.ui.form.Button("Save");
    this.add(addButton, {row: 5, column: 0, colSpan: 2});
    addButton.setToolTipText("Add/Save changes");

    addButton.addListener("execute", function() 
    {
      if (manager.validate())
      {
        var sel = fState.getSelection(); 
        var item = sel[0];
       
        var strExp = "";
        if (fExpiry.getValue() != null)
          strExp = fExpiry.getValue();
        
        if (pin_id != -1)
          this.updatePIN(pin_id, strExp, item.getModel()["$$user_id"]);
        else
          this.insertPIN(fPIN.getValue(), strExp, item.getModel()["$$user_id"], member_id)
        
        this.close();
      }
    }, this); 
   // postButton.setWidth(60);
    addButton.setEnabled(true); 
 
    if (pin_id != -1)
    {
      fPIN.setValue(pin_val);
      this.debug("pin_expiry = " + pin_expiry);
      //fExpiry.setValue(pin_expiry);
      
      
      if (pin_expiry != null)
      {
        var dformat = new qx.util.format.DateFormat("yyyy-MM-dd HH:mm:ss");      
        fExpiry.setValue(dformat.parse(pin_expiry));
      }
      
    fState.setModelSelection([state_model.getItem((pin_state-10)/10 )]);
    
    }  
  },

  members:
  {

    updatePIN : function(pin_id, expiry, state)
    {
      var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");

      try 
      {
        var res = rpc.callSync("updatepin", pin_id, expiry, state);
        if (res != "")
          alert("Failed: " + res);        
      } catch (exc) 
      {
          alert("Exception during sync call: " + exc);
      }
    },
      
    insertPIN : function(pin, expiry, state, member_id)
    {
      var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");

      try 
      {
        var res = rpc.callSync("insertpin", pin, expiry, state, member_id);
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

