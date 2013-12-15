drop procedure if exists sp_invoice_update;

/*
 * Allows generating invoices to be updated to either GENERATED or FAILED, and the email_id to be set
 */

DELIMITER //
CREATE PROCEDURE sp_invoice_update
(
  IN  invoice_id     int,
  IN  email_id       int,
  IN  invoice_status varchar(16),
  OUT err            varchar(100)   
)
SQL SECURITY DEFINER
BEGIN
  declare ck_exists int;
  set err = '';
  
  if (email_id = -1) then
    set email_id = null;
  end if;
  
  main: begin  

    if (invoice_status not in ('GENERATED','FAILED')) then
      set err = 'Invalid invoice_status';
      leave main;
    end if;
    
    select count(*) into ck_exists
    from invoices i 
    where i.invoice_id = invoice_id
      and i.invoice_status = 'GENERATING';
      
    if (ck_exists != 1) then
      set err = 'Unable to find invoice at appropriate status to update';
      leave main;
    end if;
    
    update invoices 
    set invoices.email_id = email_id, invoices.invoice_status = invoice_status
    where invoices.invoice_id = invoice_id;

  end main;

END //
DELIMITER ;
