qx.Class.define("custom.wTransactions",
{
  extend : qx.ui.window.Window,

  construct : function(member_id, handle)
  {
    if (member_id > 0)
      this.base(arguments, "Transactions " + "- [" + handle + "]" );
    else
      this.base(arguments, "Transactions");
    
    // adjust size
    this.setWidth(650);
    this.setHeight(300);

    // add the layout
    var layout = new qx.ui.layout.Grid(0, 0);
    layout.setRowFlex(1, 1);
    layout.setColumnFlex(0, 1);
    this.setLayout(layout);
    this.setContentPadding(0);
    
    var txtStatus = new qx.ui.basic.Label(this.getStatusField(member_id));
    
    this.add(txtStatus, {row: 0, column: 0});    
 
    // add grid
    var rowData = this.getRowData(member_id);
    var tableModel = new qx.ui.table.model.Simple();
    tableModel.setColumns(["transaction_datetime", "status", "type", "amount", "transaction_desc"]);
    tableModel.setData(rowData);

    var trnGrid = new qx.ui.table.Table(tableModel);

    trnGrid.set({
      width: 600,
      height: 400,
      decorator: null
    });
    
    trnGrid.setColumnWidth(0, 110);
    trnGrid.setColumnWidth(1, 70);
    trnGrid.setColumnWidth(2, 65);
    trnGrid.setColumnWidth(3, 60);
    trnGrid.setColumnWidth(4, 280);
            
    this.add(trnGrid, {row: 1, column: 0, colSpan: 1});

 
    // Transactions
    var trnButton = new qx.ui.form.Button("Manual transaction");
    this.add(trnButton, {row: 2, column: 0, colSpan: 1});
    trnButton.setToolTipText("Add manual transaction");
    trnButton.addListener("execute", function() 
    {
      
        var recTrn = new custom.wRecordTransaction(member_id);
        recTrn.moveTo(55, 35);
        recTrn.setModal(true); 
        recTrn.addListener("beforeClose", function(e) 
        {
          // refresh grid 
          tableModel.setData(this.getRowData(member_id));
          
        }, this);
        recTrn.open();
         
    }, this);
  //  trnButton.setWidth(60);
    if (qx.core.Init.getApplication().gotPermission("REC_TRAN_OWN"))
      trnButton.setEnabled(true);
    else
      trnButton.setEnabled(false);      
  },

  members:
  {

    getRowData : function(member_id)
    {
      var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");

      // get product list
      try 
      {
        var trnList = rpc.callSync("gettransactions", member_id);
      } catch (exc) 
      {
          alert("Exception during sync call: " + exc);
          return false;
      }

      var trnArr = new qx.data.Array;
      trnArr = qx.lang.Json.parse(trnList); 

      return trnArr;
    },
    
    getStatusField : function(member_id)
    {
      var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");

      // get product list
      try 
      {
        var stsTxt = rpc.callSync("gettransactionstatus", member_id);
      } catch (exc) 
      {
          alert("Exception during sync call: " + exc);
          return false;
      }

      return stsTxt;
    }
                
                
  }
  
});

