qx.Class.define("custom.wProducts",
{
  extend : qx.ui.window.Window,

  construct : function()
  {
    this.base(arguments, "Products");

    // hide the window buttons
  //  this.setShowClose(false);
  //  this.setShowMaximize(false);
  //  this.setShowMinimize(false);

    // adjust size
    this.setWidth(450);
    this.setHeight(300);

    // add the layout
    var layout = new qx.ui.layout.Grid(0, 0);
    layout.setRowFlex(0, 1);
    layout.setColumnFlex(0, 1);
    layout.setColumnFlex(1, 1);
    layout.setColumnFlex(2, 1);
    layout.setColumnFlex(3, 1);
    this.setLayout(layout);
    this.setContentPadding(0);
 
    // add grid
    var rowData = this.getRowData();
    var tableModel = new qx.ui.table.model.Simple();
    tableModel.setColumns(["ID", "Description", "Price", "Purchases"]);
    tableModel.setData(rowData);

    var prdGrid = new qx.ui.table.Table(tableModel);

    prdGrid.set({
      width: 600,
      height: 400,
      decorator: null
    });
    
    prdGrid.setColumnWidth(0, 30);
    prdGrid.setColumnWidth(1, 220);
    prdGrid.setColumnWidth(2, 60);
            
    this.add(prdGrid, {row: 0, column: 0, colSpan: 4});
 
    // Add button
    var addButton = new qx.ui.form.Button("Add");
    this.add(addButton, {row: 1, column: 0, colSpan: 1});
    addButton.setToolTipText("Add product");
    addButton.addListener("execute", function() 
    {    
        var addPrd = new custom.wAddProduct(-1);
        addPrd.moveTo(55, 35);
        addPrd.setModal(true); 
        addPrd.addListener("beforeClose", function(e) 
        {
          // refresh grid 
          tableModel.setData(this.getRowData());
          
        }, this);
        addPrd.open();
    }, this);
    addButton.setWidth(60);
    if (qx.core.Init.getApplication().gotPermission("ADD_UPD_PRODUCT"))
      addButton.setEnabled(true);
    else
      addButton.setEnabled(false);         

    // Amend button
    var amendButton = new qx.ui.form.Button("Amend");
    this.add(amendButton, {row: 1, column: 1, colSpan: 1});
    amendButton.setToolTipText("Amend product");
    amendButton.addListener("execute", function() 
    {
      
      var prdid = tableModel.getValue(0, prdGrid.getFocusedRow());

    
      var addPrd = new custom.wAddProduct(prdid);
      addPrd.moveTo(55, 35);
      addPrd.setModal(true); 
      addPrd.addListener("beforeClose", function(e) 
      {
        // refresh grid 
        tableModel.setData(this.getRowData());
          
      }, this);
      addPrd.open();
    }, this);
    amendButton.setWidth(60);
    if (qx.core.Init.getApplication().gotPermission("ADD_UPD_PRODUCT"))
      amendButton.setEnabled(true);
    else
      amendButton.setEnabled(false);    

    // Delete button
    var deleteButton = new qx.ui.form.Button("Delete [TODO]");
    this.add(deleteButton, {row: 1, column: 2, colSpan: 1});
    deleteButton.setToolTipText("Delete product");
    deleteButton.addListener("execute", function() 
    {
      // TODO: delete product
    }, this);
    deleteButton.setWidth(60);
    deleteButton.setEnabled(false); 
    
    // Sales
    var salesButton = new qx.ui.form.Button("Sales");
    this.add(salesButton, {row: 1, column: 3, colSpan: 1});
    salesButton.setToolTipText("View sales of product");
    salesButton.addListener("execute", function() 
    {      
      var prdid = tableModel.getValue(0, prdGrid.getFocusedRow());
      var prddesc = tableModel.getValue(1, prdGrid.getFocusedRow());
 
      var sales = new custom.wProductSales(prdid, prddesc);
      sales.moveTo(55, 35);
      sales.open();
    }, this);
    salesButton.setWidth(60);
    if (qx.core.Init.getApplication().gotPermission("VIEW_SALES"))
      salesButton.setEnabled(true);
    else
      salesButton.setEnabled(false);      
  
  },

  members:
  {

    getRowData : function()
    {
      var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");

      // get product list
      try 
      {
        var prdList = rpc.callSync("productlist");
      } catch (exc) 
      {
          alert("Exception during sync call: " + exc);
          return false;
      }

      var prdArr = new qx.data.Array;
      prdArr = qx.lang.Json.parse(prdList); 

      return prdArr;
    }
  },


  events :
  {
    "reload" : "qx.event.type.Event",
    "post" : "qx.event.type.Data"
  }
});

