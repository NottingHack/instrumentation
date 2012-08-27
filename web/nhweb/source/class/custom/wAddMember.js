qx.Class.define("custom.wAddMember",
{
  extend : qx.ui.window.Window,



    construct : function()
    {
      this.base(arguments, "Add Member")
 
      var layout = new qx.ui.layout.Grow();
      this.setLayout(layout);

      // create the UI ///////////////////

      // groupbox
      var groupBox = new qx.ui.groupbox.GroupBox("Member Details");
      groupBox.setWidth(200);
      this.add(groupBox);
      var grid = new qx.ui.layout.Grid();
      grid.setSpacing(5);
      grid.setColumnAlign(0, "left", "middle")
      groupBox.setLayout(grid);

      // name
      var nameLabel = new qx.ui.basic.Label("Name:");
      groupBox.add(nameLabel, {row: 0, column: 0});
      var nameTextfield = new qx.ui.form.TextField();
      groupBox.add(nameTextfield, {row: 0, column: 1});
      
      // member number
      var memberNoLabel = new qx.ui.basic.Label("Member number:");
      groupBox.add(memberNoLabel, {row: 1, column: 0});
      var memberNofield = new qx.ui.form.TextField();
      memberNofield.setFilter(/[0-9]/);
      groupBox.add(memberNofield, {row: 1, column: 1});      
      
      // email
      var emailLabel = new qx.ui.basic.Label("E-mail:");
      groupBox.add(emailLabel, {row: 2, column: 0});
      var emailfield = new qx.ui.form.TextField();
      groupBox.add(emailfield, {row: 2, column: 1});       

      // Join date
      var joinLabel = new qx.ui.basic.Label("Join date:");
      groupBox.add(joinLabel, {row: 3, column: 0});
      var joinfield = new qx.ui.form.DateField();
      joinfield.setPlaceholder("dd.mm.YYYY");
      groupBox.add(joinfield, {row: 3, column: 1});         
      
      // handle
      var handleLabel = new qx.ui.basic.Label("Handle:");
      groupBox.add(handleLabel, {row: 4, column: 0});
      var handlefield = new qx.ui.form.TextField();
      groupBox.add(handlefield, {row: 4, column: 1});       
      
      // unlock text
      var unlockLabel = new qx.ui.basic.Label("Unlock text:");
      groupBox.add(unlockLabel, {row: 5, column: 0});
      var unlockfield = new qx.ui.form.TextField();
      groupBox.add(unlockfield, {row: 5, column: 1});       
      
      // pin
      var pinLabel = new qx.ui.basic.Label("Initial PIN:");
      groupBox.add(pinLabel, {row: 6, column: 0});
      var pinfield = new qx.ui.form.TextField();
      pinfield.setFilter(/[0-9]/);
      groupBox.add(pinfield, {row: 6, column: 1});       
            

      // serialize button
      var sendButton = new qx.ui.form.Button("Send");
      groupBox.add(sendButton, {row: 7, column: 0});
      ////////////////////////////////////


      // binding /////////////////////////

      var modelSkeleton = {name: null, number: null, email: null, join: null, handle: null, unlock: null, pin: null};
      var model = qx.data.marshal.Json.createModel(modelSkeleton);

      // create the controller
      var controller = new qx.data.controller.Object(model);

      // connect the name
      controller.addTarget(nameTextfield, "value", "name", true);
      controller.addTarget(memberNofield, "value", "number", true);
      controller.addTarget(emailfield, "value", "email", true);
      controller.addTarget(joinfield, "value", "join", true);
      controller.addTarget(handlefield, "value", "handle", true);
      controller.addTarget(unlockfield, "value", "unlock", true);
      controller.addTarget(pinfield, "value", "pin", true);
 


      // validation //////////////////////

      // mark the fields as required
      nameTextfield.setRequired(true);
      memberNofield.setRequired(true);


      // create the manager
      var manager = new qx.ui.form.validation.Manager();
      nameTextfield.setRequired(true);
      manager.add(nameTextfield);
      manager.add(memberNofield);

     // Add button
     sendButton.addListener("execute", function() 
     {
       if (manager.validate()) 
       {
         try
         {
           var rpc = new qx.io.remote.Rpc(qx.core.Init.getApplication().gURL, "qooxdoo.nhweb");
           var ret = rpc.callSync("addmember", memberNofield.getValue(), nameTextfield.getValue(), handlefield.getValue(), unlockfield.getValue(), pinfield.getValue(), emailfield.getValue(), joinfield.getValue()); 
         } catch (exc) 
         {
           alert("Exception during sync call: " + exc);
           return false;
         }
         if (ret == "added")
         {
           alert("Memeber added");
           this.close();
         }
         else 
         {
           alert("Error: " + ret);
         }
       }
     }, this);
    } 

  
});

