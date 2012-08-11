qx.Class.define("custom.wAddMemberToGroup",
{
  extend : qx.ui.window.Window,

  construct : function(grp_id)
  {
    this.base(arguments, "Add member to group");

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
    var label = new qx.ui.basic.Label("Member: ");
    this.add(label, {row: 0, column: 0});

    // Add combobox
    var comboBox = new qx.ui.form.SelectBox(); 

    // get product list
    var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");
    try 
    {
      var memlst = rpc.callSync("memberlistbox");
    } catch (exc) 
    {
        alert("Exception during sync call: " + exc);
        return false;
    }

    var memArr = new qx.data.Array;
    memArr = qx.lang.Json.parse(memlst); 

    for (var i = 0; i < memArr.length; i++)
    {
      var tempItem = new qx.ui.form.ListItem(memArr[i][1]); // Member handle
      tempItem.setModel(memArr[i][0]); // member_id
      comboBox.add(tempItem);
    }

    this.add(comboBox, {row: 0, column: 1});

   
    // Ok button
    var okButton = new qx.ui.form.Button("Add");
    this.add(okButton, {row: 1, column: 0, colSpan: 2});
    okButton.setToolTipText("Add member to group");
    okButton.addListener("execute", function() 
    {
      var sel = comboBox.getSelection(); 

      // detect if selection contains something
      if (sel.length)
      {
        var mem = sel[0];

        try 
        {
          var memlst = rpc.callSync("addgroupmember", grp_id, mem.getModel());
        } catch (exc) 
        {
            alert("Exception during sync call: " + exc);
            return false;
        }    
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

