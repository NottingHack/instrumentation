qx.Class.define("custom.wMembers",
{
  extend : qx.ui.window.Window,

  construct : function()
  {
    this.base(arguments, "Members");

    // hide the window buttons
  //  this.setShowClose(false);
  //  this.setShowMaximize(false);
  //  this.setShowMinimize(false);

    // adjust size
    this.setWidth(850);
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
    tableModel.setColumns(["member_id", "Member No.", "Full Name" , "Email", "Join date", "Handle", "Unlock text", "RFID count", "PIN count"]);
    tableModel.setData(rowData);

    var memGrid = new qx.ui.table.Table(tableModel, {initiallyHiddenColumns: [0]});

    memGrid.set({
      width: 600,
      height: 400,
      decorator: null
    });
    
    
    memGrid.setColumnWidth(1, 80);
    memGrid.setColumnWidth(2, 150);
    memGrid.setColumnWidth(3, 150);
    memGrid.setColumnWidth(4, 100);
    memGrid.setColumnWidth(5, 100);
    memGrid.setColumnWidth(6, 100);
    memGrid.setColumnWidth(7, 75);
    memGrid.setColumnWidth(8, 75);
            
    this.add(memGrid, {row: 0, column: 0, colSpan: 4});
 
    // Add button
    var addButton = new qx.ui.form.Button("Add");
    this.add(addButton, {row: 1, column: 0, colSpan: 1});
    addButton.setToolTipText("Add member");
    addButton.addListener("execute", function() 
    {    
        var addMem = new custom.wAddMember();
        addMem.moveTo(55, 35);
        addMem.setModal(true); 
        addMem.addListener("beforeClose", function(e) 
        {
          // refresh grid 
          tableModel.setData(this.getRowData());
          
        }, this);
        addMem.open();
    }, this);
    addButton.setWidth(60);
    if (qx.core.Init.getApplication().gotPermission("ADD_MEMBER"))
      addButton.setEnabled(true);
    else
      addButton.setEnabled(false);         
 
    // PINs button
    var pinsButton = new qx.ui.form.Button("PINs");
    this.add(pinsButton, {row: 1, column: 1, colSpan: 1});
    pinsButton.setToolTipText("View PINs");
    pinsButton.addListener("execute", function() 
    {      
      var memid  = tableModel.getValue(0, memGrid.getFocusedRow());
      var memhan = tableModel.getValue(5, memGrid.getFocusedRow());

      var viewPins = new custom.wMemberPINS(memid, memhan);
      viewPins.moveTo(55, 35);
      viewPins.setModal(true); 
      viewPins.addListener("beforeClose", function(e) 
      {
        // refresh grid 
        tableModel.setData(this.getRowData());
          
      }, this);
      viewPins.open();
    }, this);
    pinsButton.setWidth(60);
    if (qx.core.Init.getApplication().gotPermission("VIEW_MEMBER_PINS"))
      pinsButton.setEnabled(true);
    else
      pinsButton.setEnabled(false);    

    /*
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
      var memid = tableModel.getValue(0, memGrid.getFocusedRow());
      var prddesc = tableModel.getValue(1, memGrid.getFocusedRow());
 
      var sales = new custom.wProductSales(memid, prddesc);
      sales.moveTo(55, 35);
      sales.open();
    }, this);
    salesButton.setWidth(60);
    if (qx.core.Init.getApplication().gotPermission("VIEW_SALES"))
      salesButton.setEnabled(true);
    else
      salesButton.setEnabled(false);      
  */
  },

  members:
  {

    getRowData : function()
    {
      var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");

      // get product list
      try 
      {
        var memList = rpc.callSync("memberlist");
      } catch (exc) 
      {
          alert("Exception during sync call: " + exc);
          return false;
      }

      var memArr = new qx.data.Array;
      memArr = qx.lang.Json.parse(memList); 

      return memArr;
    }
  },


  events :
  {
    "reload" : "qx.event.type.Event",
    "post" : "qx.event.type.Data"
  }
});

