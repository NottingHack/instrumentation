qx.Class.define("custom.wProductSales",
{
  extend : qx.ui.window.Window,

  construct : function(product_id, productdesc)
  {
    this.base(arguments, "Sales of " + "- [" + productdesc + "]" );

    // adjust size
    this.setWidth(400);
    this.setHeight(300);

    // add the layout
    var layout = new qx.ui.layout.Grid(0, 0);
    layout.setRowFlex(0, 1);
    layout.setColumnFlex(0, 1);
    this.setLayout(layout);
    this.setContentPadding(0);
 
    // add grid
    var rowData = this.getRowData(product_id);
    var tableModel = new qx.ui.table.model.Simple();
    tableModel.setColumns(["transaction_id", "Date/time", "Handle", "Type", "Price"]);
    tableModel.setData(rowData);

    var saleGrid = new qx.ui.table.Table(tableModel, {initiallyHiddenColumns: [0]});

    saleGrid.set({
      width: 375,
      height: 400,
      decorator: null
    });
    
    saleGrid.setColumnWidth(1, 120);
    saleGrid.setColumnWidth(2, 150);
    saleGrid.setColumnWidth(3, 60);
    saleGrid.setColumnWidth(4, 60);
            
    this.add(saleGrid, {row: 0, column: 0, colSpan: 1});       
  },

  members:
  {

    getRowData : function(product_id)
    {
      var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");

      // get product list
      try 
      {
        var salesList = rpc.callSync("getsales", product_id);
      } catch (exc) 
      {
          alert("Exception during sync call: " + exc);
          return false;
      }

      var saleArr = new qx.data.Array;
      saleArr = qx.lang.Json.parse(salesList); 

      return saleArr;
    }
  }
  
});

