qx.Class.define("custom.wGroups",
{
  extend : qx.ui.window.Window,

  construct : function()
  {
    this.base(arguments, "Groups");

    // adjust size
    this.setWidth(400);
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
    tableModel.setColumns(["ID", "Description", "Members"]);
    tableModel.setData(rowData);

    var grpGrid = new qx.ui.table.Table(tableModel);

    grpGrid.set({
      width: 600,
      height: 400,
      decorator: null
    });
    
    grpGrid.setColumnWidth(0, 50);
    grpGrid.setColumnWidth(1, 250);
    grpGrid.setColumnWidth(2, 75);    
            
    this.add(grpGrid, {row: 0, column: 0, colSpan: 4});
 
    // Members
    var memButton = new qx.ui.form.Button("Members");
    this.add(memButton, {row: 1, column: 0, colSpan: 1});
    memButton.setToolTipText("View group members");
    memButton.addListener("execute", function() 
    {    
      var grpid  = tableModel.getValue(0, grpGrid.getFocusedRow());
      var grpdsc = tableModel.getValue(1, grpGrid.getFocusedRow());
    
      var grpmem = new custom.wGroupMembers(grpid, grpdsc);
      grpmem.moveTo(55, 35);
      //grpmem.setModal(true); 
      grpmem.addListener("beforeClose", function(e) 
      {
        // refresh grid 
        tableModel.setData(this.getRowData());
          
      }, this);
      grpmem.open();
    }, this);
    
    if (qx.core.Init.getApplication().gotPermission("VIEW_GRP_MEMBERS"))
      memButton.setEnabled(true);
    else
      memButton.setEnabled(false);

    // Permissions
    var permButton = new qx.ui.form.Button("Permissions");
    this.add(permButton, {row: 1, column: 1, colSpan: 1});
    permButton.setToolTipText("View/amend group permissions");
    permButton.addListener("execute", function() 
    {
      var grpid  = tableModel.getValue(0, grpGrid.getFocusedRow());
      var grpdsc = tableModel.getValue(1, grpGrid.getFocusedRow());

    
      var gPerm = new custom.wGroupPermissions(grpid, grpdsc);
      gPerm.moveTo(55, 35);
      gPerm.addListener("beforeClose", function(e) 
      {
        // refresh grid 
        tableModel.setData(this.getRowData());
          
      }, this);
      gPerm.open();

    }, this);
    
    if (qx.core.Init.getApplication().gotPermission("VIEW_GRP_PERMIS"))
      permButton.setEnabled(true);
    else
      permButton.setEnabled(false);    
    
    // Add Group
    var addButton = new qx.ui.form.Button("Add");
    this.add(addButton, {row: 1, column: 2, colSpan: 1});
    addButton.setToolTipText("Add group");
    addButton.addListener("execute", function() 
    {
    
      var addG = new custom.wAddGroup();
      addG.moveTo(55, 35);
      addG.setModal(true); 
      addG.open();
      addG.addListener("beforeClose", function(e) 
      {
        // refresh grid 
        tableModel.setData(this.getRowData());
          
      }, this);      

    }, this);
    
    if (qx.core.Init.getApplication().gotPermission("ADD_GROUP"))
      addButton.setEnabled(true);
    else
      addButton.setEnabled(false);     
    
    // Delete Group
    var delButton = new qx.ui.form.Button("Delete");
    this.add(delButton, {row: 1, column: 3, colSpan: 1});
    delButton.setToolTipText("Delete group");
    delButton.addListener("execute", function() 
    {
      var grpid  = tableModel.getValue(0, grpGrid.getFocusedRow());
      var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");

      try 
      {
        var grpList = rpc.callSync("deletegroup", grpid);
      } catch (exc) 
      {
          alert("Exception during sync call: " + exc);
          return false;
      }
      
      tableModel.setData(this.getRowData());
      
    }, this); 
    if (qx.core.Init.getApplication().gotPermission("DEL_GROUP"))
      delButton.setEnabled(true);
    else
      delButton.setEnabled(false);        
        
  },

  members:
  {

    getRowData : function()
    {
      var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");

      try 
      {
        var grpList = rpc.callSync("getgroups");
      } catch (exc) 
      {
          alert("Exception during sync call: " + exc);
          return false;
      }

      var grpArr = new qx.data.Array;
      grpArr = qx.lang.Json.parse(grpList); 

      return grpArr;
    }
  }
  
});

