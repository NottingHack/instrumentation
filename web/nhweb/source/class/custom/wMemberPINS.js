qx.Class.define("custom.wMemberPINS",
{
  extend : qx.ui.window.Window,

  construct : function(member_id, handle)
  {
    this.base(arguments, "PINs for " + "[" + handle + "]" );

    // adjust size
    this.setWidth(400);
    this.setHeight(300);

    // add the layout
    var layout = new qx.ui.layout.Grid(0, 0);
    layout.setRowFlex(0, 1);
    layout.setColumnFlex(0, 1);
    layout.setColumnFlex(1, 1);
    this.setLayout(layout);
    this.setContentPadding(0);
 
    // PINs grid
    var rowData = this.getRowData(member_id);
    var tableModel = new qx.ui.table.model.Simple();
    tableModel.setColumns(["pin_id", "state", "PIN", "State", "Date added", "Expiry"]);
    tableModel.setData(rowData);

    var pinGrid = new qx.ui.table.Table(tableModel, {initiallyHiddenColumns: [0, 1]});

    pinGrid.set({
      width: 375,
      height: 400,
      decorator: null
    });
    
    pinGrid.setColumnWidth(2, 60);
    pinGrid.setColumnWidth(3, 80);
    pinGrid.setColumnWidth(4, 120);
    pinGrid.setColumnWidth(5, 120);
            
    this.add(pinGrid, {row: 0, column: 0, colSpan: 2}); 
    
    
 
    // Add PIN
    var addButton = new qx.ui.form.Button("Add");
    this.add(addButton, {row: 1, column: 0, colSpan: 1});
    addButton.setToolTipText("Add PIN");
    addButton.addListener("execute", function() 
    {    
        var addMem = new custom.wPINUpdate(member_id, -1);
        addMem.moveTo(55, 35);
        addMem.setModal(true); 
        addMem.addListener("beforeClose", function(e) 
        {
          // refresh grid 
          tableModel.setData(this.getRowData(member_id));
          
        }, this);
        addMem.open();
    }, this);
    addButton.setWidth(60);
    if (qx.core.Init.getApplication().gotPermission("AMEND_PINS"))
      addButton.setEnabled(true);
    else
      addButton.setEnabled(false);         
 
    // Amend PIN
    var amendButton = new qx.ui.form.Button("Update");
    this.add(amendButton, {row: 1, column: 1, colSpan: 1});
    amendButton.setToolTipText("Update selected PIN");
    amendButton.addListener("execute", function() 
    {      
      var pin_id      = tableModel.getValue(0, pinGrid.getFocusedRow());
      var pin_state   = tableModel.getValue(1, pinGrid.getFocusedRow());
      var pin_val     = tableModel.getValue(2, pinGrid.getFocusedRow());
      var pin_expiry  = tableModel.getValue(5, pinGrid.getFocusedRow());
       
      var amendPins = new custom.wPINUpdate(member_id, pin_id, pin_val, pin_expiry, pin_state);
      amendPins.moveTo(55, 35);
      amendPins.setModal(true); 
      amendPins.addListener("beforeClose", function(e) 
      {
        // refresh grid 
        tableModel.setData(this.getRowData(member_id));
          
      }, this);
      amendPins.open();
    }, this);
    amendButton.setWidth(60);
    if (qx.core.Init.getApplication().gotPermission("AMEND_PINS"))
      amendButton.setEnabled(true);
    else
      amendButton.setEnabled(false);    
    
    
    
  },

  members:
  {

    getRowData : function(member_id)
    {
      var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");

      // get product list
      try 
      {
        var pinsList = rpc.callSync("viewpins", member_id);
      } catch (exc) 
      {
          alert("Exception during sync call: " + exc);
          return false;
      }

      var pinArr = new qx.data.Array;
      pinArr = qx.lang.Json.parse(pinsList); 

      return pinArr;
    }
  }
  
});

