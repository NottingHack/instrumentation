qx.Class.define("custom.wRecordTransaction",
{
  extend : qx.ui.window.Window,

  construct : function(member_id)
  {
    this.base(arguments, "Record Transaction");

    // hide the window buttons
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
    this.add(new qx.ui.basic.Label("Enter amount in Pence. +ve to credit account (payment to snackspace),"), {row: 0, column: 0, colSpan: 2}); 
    this.add(new qx.ui.basic.Label("-ve to debit account (record purchace)"), {row: 1, column: 0, colSpan: 2});
    this.add(new qx.ui.basic.Label("Description "), {row: 2, column: 0});
    this.add(new qx.ui.basic.Label("Amount "), {row: 3, column: 0}); 

    // Description entry
    var txtDesc = new qx.ui.form.TextField();
    this.add(txtDesc, {row: 2, column: 1});

    // Amount
    var spnAmount = new qx.ui.form.Spinner();
    spnAmount.setMaximum(100000);
    spnAmount.setMinimum(-100000);
    spnAmount.setValue(0);
    this.add(spnAmount, {row: 3, column: 1});  
  
      
    var modelSkeleton = {description: null, amount: null};
    var model = qx.data.marshal.Json.createModel(modelSkeleton);

    // create the controller
    var controller = new qx.data.controller.Object(model);

    // connect the name
    controller.addTarget(txtDesc, "value", "description", true);
    controller.addTarget(spnAmount, "value", "amount", true);
    txtDesc.setRequired(true);
    spnAmount.setRequired(true);

    // create the manager
    var manager = new qx.ui.form.validation.Manager();
    manager.add(txtDesc);
    manager.add(spnAmount);


   
    // post button
    var postButton = new qx.ui.form.Button("Record");
    this.add(postButton, {row: 4, column: 0, colSpan: 2});
    postButton.setToolTipText("Record transaction");

    postButton.addListener("execute", function() 
    {
      if (manager.validate())
      {
        
        var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");
        try 
        {
          rpc.callSync("recordtran", spnAmount.getValue(), txtDesc.getValue(), member_id);
        } catch (exc) 
        {
            alert("Exception during sync call: " + exc);
            return false;
        }
        this.close();
      }
    }, this); 
    postButton.setWidth(60);
    postButton.setEnabled(true);
  },

  members:
  {

   
  }
});

