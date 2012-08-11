qx.Class.define("custom.wAddGroup",
{
  extend : qx.ui.window.Window,

  construct : function()
  {
    this.base(arguments, "Add Group");

    // hide the window button
    this.setShowMaximize(false);
    this.setShowMinimize(false);

    // adjust size
    this.setWidth(250);
    this.setHeight(50);

    // add the layout
    var layout = new qx.ui.layout.Grid(0, 0);
    layout.setRowFlex(0, 1);
    layout.setColumnFlex(1, 1);
    this.setLayout(layout);
    this.setContentPadding(0);

    // Add label
    var label = new qx.ui.basic.Label("Group name: ");
    this.add(label, {row: 0, column: 0});

    // Add description box
    var txtdesc = new qx.ui.form.TextField();
    this.add(txtdesc, {row: 0, column: 1});


   
    // Add button
    var addButton = new qx.ui.form.Button("Add");
    this.add(addButton, {row: 1, column: 0, colSpan: 2});
    addButton.setToolTipText("Add group");
    addButton.addListener("execute", function() 
    {
      try
      {
        var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");
        rpc.callSync("addgroup", txtdesc.getValue());
      } catch (exc) 
      {
        alert("Exception during sync call: " + exc);
        return false;
      }    
      this.close();
    }, this);
    addButton.setWidth(60);
    addButton.setEnabled(true);
  }
  
});

