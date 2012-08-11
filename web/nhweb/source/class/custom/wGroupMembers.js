qx.Class.define("custom.wGroupMembers",
{
  extend : qx.ui.window.Window,

  construct : function(grpid, grpdsc)
  {
    this.base(arguments, "Group members " + "- [" + grpdsc + "]" );

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
    var rowData = this.getRowData(grpid);
    var tableModel = new qx.ui.table.model.Simple();
    tableModel.setColumns(["member_id", "Handle", "Member number"]);
    tableModel.setData(rowData);

    var memberGrid = new qx.ui.table.Table(tableModel, {initiallyHiddenColumns: [0]});

    memberGrid.set({
      width: 600,
      height: 400,
      decorator: null
    });
    
    memberGrid.setColumnWidth(1, 200);
    memberGrid.setColumnWidth(2, 100);
    
            
    this.add(memberGrid, {row: 0, column: 0, colSpan: 2});
 
    // Add
    var addMemButton = new qx.ui.form.Button("Add");
    this.add(addMemButton, {row: 1, column: 0});
    addMemButton.setToolTipText("Add member to group");
    addMemButton.addListener("execute", function() 
    {        
      var grpmem = new custom.wAddMemberToGroup(grpid);
      grpmem.moveTo(55, 35);
      //grpmem.setModal(true); 
      grpmem.addListener("beforeClose", function(e) 
      {
        // refresh grid 
        tableModel.setData(this.getRowData(grpid));
          
      }, this);
      grpmem.open();
    }, this);
  //addMemButton.setWidth(60);
    if (qx.core.Init.getApplication().gotPermission("ADD_GRP_MEMBER"))
      addMemButton.setEnabled(true);
    else
      addMemButton.setEnabled(false);      

    // Remove Member
    var delMemButton = new qx.ui.form.Button("Remove");
    this.add(delMemButton, {row: 1, column: 1});
    delMemButton.setToolTipText("Remove selected member from group");
    delMemButton.addListener("execute", function() 
    {
      var memberid = tableModel.getValue(0, memberGrid.getFocusedRow());

      this.removeMemberFromGroup(grpid, memberid);
      
      // refresh grid 
      tableModel.setData(this.getRowData(grpid));
    }, this);
  //delMemButton.setWidth(60);
    if (qx.core.Init.getApplication().gotPermission("REM_GRP_MEMBER"))
      delMemButton.setEnabled(true);
    else
      delMemButton.setEnabled(false);     
        
  },

  members:
  {

    getRowData : function(grpid)
    {
      var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");

      try 
      {
        var grpList = rpc.callSync("getgroupmembers", grpid);
      } catch (exc) 
      {
          alert("Exception during sync call: " + exc);
          return false;
      }

      var grpArr = new qx.data.Array;
      grpArr = qx.lang.Json.parse(grpList); 

      return grpArr;
    },
                
    removeMemberFromGroup : function(grpid, member_id)
    {
      var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");

      try 
      {
        var grpList = rpc.callSync("removegroupmember", grpid, member_id);
      } catch (exc) 
      {
          alert("Exception during sync call: " + exc);
          return false;
      }
    }                
                
  }
  
});

