qx.Class.define("custom.wGetProduct",
{
  extend : qx.ui.window.Window,

  construct : function(loc)
  {
    this.base(arguments, "Select Product");

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
//    var label = new qx.ui.basic.Label("Product:");
    var label = new qx.ui.basic.Label("Product for [" + loc + "]:");
    this.add(label, {row: 0, column: 0});

    // Add combobox
    var comboBox = new qx.ui.form.SelectBox();


    // Add empty/blank product
    var tempItem = new qx.ui.form.ListItem("<empty>"); // product description
    tempItem.setModel(-1); // product id
    comboBox.add(tempItem);    

    // get product list
    var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");
    try 
    {
      var prdlst = rpc.callSync("productlistbox");
    } catch (exc) 
    {
        alert("Exception during sync call: " + exc);
        return false;
    }

    var prdArr = new qx.data.Array;
    prdArr = qx.lang.Json.parse(prdlst); 

    for (var i = 0; i < prdArr.length; i++)
    {
      var tempItem = new qx.ui.form.ListItem(prdArr[i][1]); // product description
      tempItem.setModel(prdArr[i][0]); // product id
      comboBox.add(tempItem);
    }

    this.add(comboBox, {row: 0, column: 1});

   
    // Ok button
    var okButton = new qx.ui.form.Button("Ok");
    this.add(okButton, {row: 1, column: 0, colSpan: 2});
    okButton.setToolTipText("Ok");
    okButton.addListener("execute", function() 
    {
      var sel = comboBox.getSelection(); 

      // detect if selection contains something
      if (sel.length)
      {
        var item = sel[0];

        try 
        {
          var prdlst = rpc.callSync("setvendprd", loc, item.getModel());
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

    getComboData: function()
    {
      var rowData = [];
      rowData.push(["A1", "prd1"]);
      rowData.push(["A2", "prd2"]);
      rowData.push(["A3", "prd3"]);

      return rowData;

    }
  }
});

