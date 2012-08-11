qx.Class.define("custom.wGroupPermissions",
{
  extend : qx.ui.window.Window,

  construct : function(grpid, grpdsc)
  {
    this.base(arguments, "Group permissions " + "- [" + grpdsc + "]" );

    // adjust size
    this.setWidth(700);
    this.setHeight(500);

    // add the layout
    var layout = new qx.ui.layout.Grid(0, 0);
    layout.setRowFlex(0, 1);
    layout.setColumnFlex(0, 1);
    this.setLayout(layout);
    this.setContentPadding(0);
 
    // add grid
    var rowData = this.getRowData(grpid);
    var tableModel = new qx.ui.table.model.Simple();
    tableModel.setColumns(["Permission code", "Description", "State"]);
    tableModel.setData(rowData);

    var permissionsGrid = new qx.ui.table.Table(tableModel);

    permissionsGrid.set({
      width: 800,
      height: 400,
      decorator: null
    });
    permissionsGrid.setColumnWidth(0, 130);
    permissionsGrid.setColumnWidth(1, 400);
    permissionsGrid.setColumnWidth(2, 70);
    
    this.add(permissionsGrid, {row: 0, column: 0, colSpan: 1});

   
    // Toggle permissions button
    var ptoggleButton = new qx.ui.form.Button("Toggle");
    this.add(ptoggleButton, {row: 1, column: 0, colSpan: 1});
    ptoggleButton.setToolTipText("Toggle selected permissions");
    ptoggleButton.addListener("execute", function() 
    {
      var pCode = tableModel.getValue(0, permissionsGrid.getFocusedRow());
      
      var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");

      try 
      {
        var pdata = rpc.callSync("togglegrouppermission", grpid, pCode);
      } catch (exc) 
      {
          alert("Exception during sync call: " + exc);
          return false;
      }

      tableModel.setData(this.getRowData(grpid));
    }, this);
    ptoggleButton.setWidth(60);
    if (qx.core.Init.getApplication().gotPermission("CHG_GRP_PERM"))
      ptoggleButton.setEnabled(true);
    else
      ptoggleButton.setEnabled(false);           

  },

  members:
  {

    getRowData : function(grpid)
    {

      var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");

      // Permissions
      try 
      {
        var pdata = rpc.callSync("getgrouppermissions", grpid);
      } catch (exc) 
      {
          alert("Exception during sync call: " + exc);
          return false;
      }

      var parr = new qx.data.Array;
      parr = qx.lang.Json.parse(pdata); 
      return parr;

    }
  }
  
});

