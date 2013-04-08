drop procedure if exists sp_set_credit_limit;

/*

*/

DELIMITER //
CREATE PROCEDURE sp_set_credit_limit 
(
   IN   member_id     int,
   IN   credit_limit  int,
   OUT  err           varchar(100)
)
SQL SECURITY DEFINER
BEGIN
 
  main: begin  

    set err = '';

    update members m
    set m.credit_limit = credit_limit
    where m.member_id = member_id;

    
  end main;
  

END //
DELIMITER ;

