drop procedure if exists sp_web_updateproduct;

/*

*/

DELIMITER //
CREATE PROCEDURE sp_web_updateproduct 
(
   IN   member_id  int,
   IN   product_id int,
   IN   price      int,
   IN   barcode    varchar(25),
   IN   available  int,
   IN   shortdesc  varchar(25),
   IN   longdesc   varchar(512),
   OUT  err        varchar(100)
)
SQL SECURITY DEFINER
BEGIN
 
  main: begin  

    set err = '';

    if (product_id = -1) then
      insert into products (price, barcode, available, shortdesc, longdesc) values (price, barcode, available, shortdesc, longdesc);
    else
      replace into products (product_id, price, barcode, available, shortdesc, longdesc) values (product_id, price, barcode, available, shortdesc, longdesc);
    end if;
    
  end main;
  

END //
DELIMITER ;

