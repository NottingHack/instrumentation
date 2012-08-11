qx.Class.define("custom.wAddProduct",
{
  extend : qx.ui.window.Window,

  construct : function(product_id)
  {
    if (product_id == -1) 
      this.base(arguments, "Add Product");
    else
      this.base(arguments, "Amend Product");

    this.setShowClose(true);
    this.setShowMaximize(false);
    this.setShowMinimize(false);

    // adjust size
   // this.setWidth(250);
   // this.setHeight(300);

    // add the layout
    var layout = new qx.ui.layout.Grid(0, 0);
    layout.setRowFlex(1, 1);
    layout.setColumnFlex(1, 1);
    this.setLayout(layout);
    this.setContentPadding(0);
 
    // add labels
    this.add(new qx.ui.basic.Label("Short Description: "), {row: 0, column: 0});
    this.add(new qx.ui.basic.Label("Full Description: ") , {row: 1, column: 0}); 
    this.add(new qx.ui.basic.Label("Price (p): ")        , {row: 2, column: 0}); 
    this.add(new qx.ui.basic.Label("Barcode: ")          , {row: 3, column: 0}); 
    this.add(new qx.ui.basic.Label("Available: ")        , {row: 4, column: 0}); 

    // Details entry
    var txtShortdesc = new qx.ui.form.TextField();
    var txtFulldesc  = new qx.ui.form.TextField();
    var txtPrice     = new qx.ui.form.TextField();
    var txtBarcode   = new qx.ui.form.TextField();
    var selAvailable = new qx.ui.form.SelectBox();

    txtPrice.setFilter(/[0-9]/);

    // Combo box options
    var tempItem = new qx.ui.form.ListItem("Yes");
    tempItem.setModel(1);  
    selAvailable.add(tempItem);  
    var tempItem = new qx.ui.form.ListItem("No"); 
    tempItem.setModel(0); 
    selAvailable.add(tempItem);  

    
    this.add(txtShortdesc, {row: 0, column: 1});
    this.add(txtFulldesc , {row: 1, column: 1});
    this.add(txtPrice    , {row: 2, column: 1});
    this.add(txtBarcode  , {row: 3, column: 1});
    this.add(selAvailable, {row: 4, column: 1});
    
    var modelSkeleton = {Shortdesc: null, Fulldesc: null, Price: null, Barcode: null, Available: null};
    var model = qx.data.marshal.Json.createModel(modelSkeleton);

    // create the controller
    var controller = new qx.data.controller.Object(model);
  
    // connect the name
    controller.addTarget(txtShortdesc, "value", "Shortdesc", true);   
    controller.addTarget(txtFulldesc , "value", "Fulldesc" , true);
    controller.addTarget(txtPrice    , "value", "Price"    , true);
    controller.addTarget(txtBarcode  , "value", "Barcode"  , true);

    txtShortdesc.setRequired(true);
    txtFulldesc.setRequired(true);
    txtPrice.setRequired(true);
    txtBarcode.setRequired(false);
    selAvailable.setRequired(true);


    // create the manager
    var manager = new qx.ui.form.validation.Manager();
    manager.add(txtShortdesc);
    manager.add(txtFulldesc);
    manager.add(txtPrice);
    manager.add(txtBarcode);
    manager.add(selAvailable);


    // Add button
    var addButton = new qx.ui.form.Button("Save");
    this.add(addButton, {row: 5, column: 0, colSpan: 2});
    addButton.setToolTipText("Add/Save changes");

    addButton.addListener("execute", function() 
    {
      if (manager.validate())
      {
        var sel = selAvailable.getSelection(); 
        var item = sel[0];
        
        this.saveProduct(product_id, txtPrice.getValue(), txtBarcode.getValue(), item.getModel(), txtShortdesc.getValue(), txtFulldesc.getValue());
        this.close();
      }
    }, this); 
   // postButton.setWidth(60);
    addButton.setEnabled(true); 

    if (product_id != -1)
    {
      var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");
     
      // get product details
      try 
      {
        var prdDetails = rpc.callSync("productdetails", product_id);
      } catch (exc) 
      {
          alert("Exception during sync call: " + exc);
          return false;
      }

      var prdArr = new qx.data.Array;
      prdArr = qx.lang.Json.parse(prdDetails);       

      txtPrice.setValue(prdArr[0].toString());
      txtBarcode.setValue(prdArr[1]);
   // selAvailable.setValue(prdArr[2]);
      txtShortdesc.setValue(prdArr[3]);
      txtFulldesc.setValue(prdArr[4]);
    } 
  },

  members:
  {

    saveProduct : function(product_id, price, barcode, available, shortDesc, fullDesc)
    {


      var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");

      try 
      {
        var res = rpc.callSync("updateproduct", product_id, price, barcode, available, shortDesc, fullDesc);
      } catch (exc) 
      {
          alert("Exception during sync call: " + exc);

      }

      return;
    }
  }
});

