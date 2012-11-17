qx.Class.define("custom.wMemberAccess",
{
  extend : qx.ui.window.Window,

  construct : function()
  {
    this.base(arguments, "Members - web access");

    // adjust size
    this.setWidth(550);
    this.setHeight(300);

    // add the layout
    var layout = new qx.ui.layout.Grid(0, 0);
    layout.setRowFlex(0, 1);
    layout.setColumnFlex(0, 1);
    
    this.setLayout(layout);
    this.setContentPadding(0);
 
    // add grid
    var rowData = this.getRowData();
    var tableModel = new qx.ui.table.model.Simple();
    tableModel.setColumns(["member_id", "Name", "Handle", "Username", "Password set", "Logon enabled"]);
    tableModel.setData(rowData);

    var memGrid = new qx.ui.table.Table(tableModel, {initiallyHiddenColumns: [0]});

    memGrid.set({
      width: 600,
      height: 550,
      decorator: null
    });
    
    memGrid.setColumnWidth(1, 120);
    memGrid.setColumnWidth(2, 95);
    memGrid.setColumnWidth(3, 95);
    memGrid.setColumnWidth(4, 95);
            
    this.add(memGrid, {row: 0, column: 0, colSpan: 1});
 
    // Set password
    var setPasswd = new qx.ui.form.Button("Set Password");
    this.add(setPasswd, {row: 1, column: 0, colSpan: 1});
    setPasswd.setToolTipText("Set members password");
    setPasswd.addListener("execute", function() 
    {    
      var memberid = tableModel.getValue(0, memGrid.getFocusedRow());
    
      var setPw = new custom.wChangePassword(memberid);
      setPw.moveTo(55, 35);
      setPw.setModal(true); 
      setPw.addListener("beforeClose", function(e) 
      {
        // refresh grid 
        tableModel.setData(this.getRowData());
          
      }, this);
      setPw.open();
    }, this);

    if (qx.core.Init.getApplication().gotPermission("SET_PASSWORD"))
      setPasswd.setEnabled(true);
    else
      setPasswd.setEnabled(false);    
  
        
  },

  members:
  {

    getRowData : function()
    {
      var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");

      // get product list
      try 
      {
        var memList = rpc.callSync("getaccessmemberlist");
      } catch (exc) 
      {
          alert("Exception during sync call: " + exc);
          return false;
      }

      var memArr = new qx.data.Array;
      memArr = qx.lang.Json.parse(memList); 

      return memArr;
    }
  }
  
});

