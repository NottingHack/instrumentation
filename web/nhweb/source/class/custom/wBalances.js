qx.Class.define("custom.wBalances",
{
  extend : qx.ui.window.Window,

  construct : function()
  {
    this.base(arguments, "Balances");

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
 
    // add grid
    var rowData = this.getRowData();
    var tableModel = new qx.ui.table.model.Simple();
    tableModel.setColumns(["member_id", "Handle", "Balance", "Credit Limit", "clim_int"]);
    tableModel.setData(rowData);

    var balGrid = new qx.ui.table.Table(tableModel, {initiallyHiddenColumns: [0, 4]});

    balGrid.set({
      width: 600,
      height: 400,
      decorator: null
    });
    
    balGrid.setColumnWidth(1, 100);
    balGrid.setColumnWidth(2, 80);
    balGrid.setColumnWidth(3, 80);
            
    this.add(balGrid, {row: 0, column: 0, colSpan: 2});
 
    // Transactions
    var trnButton = new qx.ui.form.Button("Transactions");
    this.add(trnButton, {row: 1, column: 0, colSpan: 1});
    trnButton.setToolTipText("View transactions");
    trnButton.addListener("execute", function() 
    {    
      var memberid = tableModel.getValue(0, balGrid.getFocusedRow());
      var handle = tableModel.getValue(1, balGrid.getFocusedRow());

    
      var memtrn = new custom.wTransactions(memberid, handle);
      memtrn.moveTo(55, 35);
      //memtrn.setModal(true); 
      memtrn.addListener("beforeClose", function(e) 
      {
        // refresh grid 
        tableModel.setData(this.getRowData());
          
      }, this);
      memtrn.open();
    }, this);

    if (qx.core.Init.getApplication().gotPermission("REC_TRAN"))
      trnButton.setEnabled(true);
    else
      trnButton.setEnabled(false);    

    // Credit limit
    var limButton = new qx.ui.form.Button("Set Limit");
    this.add(limButton, {row: 1, column: 1, colSpan: 1});
    limButton.setToolTipText("Set/amend credit limit");
    limButton.addListener("execute", function() 
    {
      var memberid = tableModel.getValue(0, balGrid.getFocusedRow());
      var current_lim = tableModel.getValue(4, balGrid.getFocusedRow());

    
      var cLim = new custom.wSetCreditLimit(memberid, current_lim);
      cLim.moveTo(55, 35);
      cLim.setModal(true); 
      cLim.addListener("beforeClose", function(e) 
      {
        // refresh grid 
        tableModel.setData(this.getRowData());
          
      }, this);
      cLim.open();

    }, this);
    
    if (qx.core.Init.getApplication().gotPermission("SET_CREDIT_LIMIT"))
      limButton.setEnabled(true);
    else
      limButton.setEnabled(false);     
        
  },

  members:
  {

    getRowData : function()
    {
      var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");

      // get product list
      try 
      {
        var balList = rpc.callSync("getbalances");
      } catch (exc) 
      {
          alert("Exception during sync call: " + exc);
          return false;
      }

      var balArr = new qx.data.Array;
      balArr = qx.lang.Json.parse(balList); 

      return balArr;
    }
  }
  
});

