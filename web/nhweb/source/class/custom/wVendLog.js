qx.Class.define("custom.wVendLog",
{
  extend : qx.ui.window.Window,

  construct : function()
  {
    this.base(arguments, "Vend Log");

    // hide the window buttons
  //  this.setShowClose(false);
  //  this.setShowMaximize(false);
  //  this.setShowMinimize(false);

    // adjust size
    this.setWidth(880);
    this.setHeight(300);

    // add the layout
    var layout = new qx.ui.layout.Grid(0, 0);
    layout.setRowFlex(1, 1);
    layout.setColumnFlex(1, 1, 1);
    this.setLayout(layout);
    this.setContentPadding(0);
 


    var tableModel = new custom.wVendLogTable();
    tableModel.setColumns(["vend_tran_id","Handle","success_datetime","cancelled_datetime","amount","position","product","denied_reason"], ["vend_tran_id","handle","success_datetime","cancelled_datetime","amount","position","product","denied_reason"]     );

    var vendLogGrid = new qx.ui.table.Table(tableModel);
    

    vendLogGrid.set({
      width: 600,
      height: 400,
      decorator: null
    });

 
    vendLogGrid.setColumnWidth(0, 80);
    vendLogGrid.setColumnWidth(1, 100);
    vendLogGrid.setColumnWidth(2, 120);
    vendLogGrid.setColumnWidth(3, 120);
    vendLogGrid.setColumnWidth(4, 70);
    vendLogGrid.setColumnWidth(5, 60);
    vendLogGrid.setColumnWidth(6, 100);
    vendLogGrid.setColumnWidth(7, 200);

      
    this.add(vendLogGrid, {row: 0, column: 0, colSpan: 3});
 


        
  },

  members:
  {

  },


  events :
  {
    "reload" : "qx.event.type.Event",
    "post" : "qx.event.type.Data"
  }
});

