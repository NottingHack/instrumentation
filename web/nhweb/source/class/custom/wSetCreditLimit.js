qx.Class.define("custom.wSetCreditLimit",
{
  extend : qx.ui.window.Window,

  construct : function(member_id, current_lim)
  {
    this.base(arguments, "Set credit limit");

    // hide the window buttons
  //  this.setShowClose(false);
  //  this.setShowMaximize(false);
  //  this.setShowMinimize(false);

    // adjust size
    this.setWidth(250);
    this.setHeight(50);

    // add the layout
    var layout = new qx.ui.layout.Grid(0, 0);
    layout.setRowFlex(1, 1);
    layout.setColumnFlex(1, 1);
    this.setLayout(layout);
    this.setContentPadding(0);

    // Add label
    var label = new qx.ui.basic.Label("Set Limit (p)");
    this.add(label, {row: 0, column: 0});

    // Add spinner
    var s = new qx.ui.form.Spinner();
    s.setMaximum(100000);
    s.setMinimum(0);
    s.setValue(parseInt(current_lim));
  //  var nf = new qx.util.format.NumberFormat();
  //  nf.setMaximumFractionDigits(0);
  //  s.setNumberFormat(nf);
    this.add(s, {row: 0, column: 1});


   
    // Ok button
    var okButton = new qx.ui.form.Button("Ok");
    this.add(okButton, {row: 1, column: 0, colSpan: 2});
    okButton.setToolTipText("Ok");
    okButton.addListener("execute", function() 
    {
      try
      {
        var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");
        rpc.callSync("setclimit", member_id, s.getValue());
      } catch (exc) 
      {
        alert("Exception during sync call: " + exc);
        return false;
      }    
      this.close();
    }, this);
    okButton.setWidth(60);
    okButton.setEnabled(true);
  },

  members:
  {

  }
});

