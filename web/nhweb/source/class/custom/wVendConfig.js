qx.Class.define("custom.wVendConfig",
{
  extend : qx.ui.window.Window,

  construct : function()
  {
    this.base(arguments, "Vending machine config");

    // hide the window buttons
  //  this.setShowClose(false);
  //  this.setShowMaximize(false);
  //  this.setShowMinimize(false);

    // adjust size
    this.setWidth(250);
    this.setHeight(300);

    // add the layout
    var layout = new qx.ui.layout.Grid(0, 0);
    layout.setRowFlex(1, 1);
    layout.setColumnFlex(0, 1);
    this.setLayout(layout);
    this.setContentPadding(0);
 
    // add grid
    var rowData = this.getRowData();
    var tableModel = new qx.ui.table.model.Simple();
    tableModel.setColumns(["Position", "Product"]);
    tableModel.setData(rowData);
    tableModel.setColumnEditable(1, false);
    tableModel.setColumnEditable(2, true);    

    var vendGrid = new qx.ui.table.Table(tableModel);

    vendGrid.set({
      width: 600,
      height: 400,
      decorator: null
    });

    this.add(vendGrid, {row: 0, column: 0, colSpan: 2});
 
    


   
     // post button
    var postButton = new qx.ui.form.Button("Amend");
    this.add(postButton, {row: 1, column: 0, colSpan: 2});
    postButton.setToolTipText("Amend selection");
    postButton.addListener("execute", function() 
    {

       var loc = tableModel.getValue(0, vendGrid.getFocusedRow());

    
        var gprd = new custom.wGetProduct(loc);
        gprd.moveTo(55, 35);
        gprd.setModal(true); 
        gprd.addListener("beforeClose", function(e) 
        {
          // refresh grid 
          tableModel.setData(this.getRowData());
          
        }, this);
        gprd.open();
    }, this);
    postButton.setWidth(60);

    if (qx.core.Init.getApplication().gotPermission("UPD_VEND_CONFIG"))
      postButton.setEnabled(true);
    else
      postButton.setEnabled(false);      
  },

  members:
  {

    getRowData : function()
    {

      var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");


      // get vend data
      try 
      {
        var venddata = rpc.callSync("getvendconfig");
      } catch (exc) 
      {
          alert("Exception during sync call: " + exc);
          return false;
      }

    // var jvend = qx.io.Json.parse(venddata);

      var test = new qx.data.Array;
      test = qx.lang.Json.parse(venddata); 


    /*
      //TODO: get data from server
      var rowData = [];
      rowData.push(["A1", "prd1"+ Math.round(Math.random()*100000000)]);
      rowData.push(["A2", "prd2"+ Math.round(Math.random()*100000000)]);
      rowData.push(["A3", "prd3"+ Math.round(Math.random()*100000000)]);
*/
      return test;

    }
  },


  events :
  {
    "reload" : "qx.event.type.Event",
    "post" : "qx.event.type.Data"
  }
});

