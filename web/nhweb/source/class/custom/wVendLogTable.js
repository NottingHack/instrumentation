qx.Class.define("custom.wVendLogTable",
{
  extend : qx.ui.table.model.Remote,

  members :
  {
     // overloaded - called whenever the table requests the row count
    _loadRowCount : function()
    {

      var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");


      try 
      {
        var result = rpc.callSync("vendlog_rowcount");
      } catch (exc) 
      {
          alert("Exception during sync call: " + exc);
          return false;
      }
      
      this._onRowCountLoaded(result);
    },


    

    // overloaded - called whenever the table requests new data
    _loadRowData : function(firstRow, lastRow)
    {
      var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");


      try 
      {
        var result = rpc.callSync("vendlog", firstRow, lastRow);
      } catch (exc) 
      {
          alert("Exception during sync call: " + exc);
          return false;
      }
      this.debug(result);
      this._onRowDataLoaded( qx.lang.Json.parse(result));

    }
  }
});
