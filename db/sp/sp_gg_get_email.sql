drop procedure if exists sp_gg_get_email;

DELIMITER //
CREATE PROCEDURE sp_gg_get_email
(
   OUT ggemail_id   int,
   OUT ggemail_body varchar(10000)
)
SQL SECURITY DEFINER
BEGIN

  declare ck_exists int;
  
  select count(*)
  into ck_exists
  from gg_email ggm
  left outer join gg_summary ggs on ggs.ggemail_id = ggm.ggemail_id
  where ggs.ggsum_auto_wc is null;

  -- get first email that doesn't have a wordcount set
  -- (regardless if it has a summary row or not)
  if (ck_exists > 0) then
    select ggm.ggemail_id, ggm.ggemail_body
    into ggemail_id, ggemail_body
    from gg_email ggm
    left outer join gg_summary ggs on ggs.ggemail_id = ggm.ggemail_id
    where ggs.ggsum_auto_wc is null
    order by ggm.ggemail_id
    limit 1;
  else
    set ggemail_id = -1;
  end if;


END //
DELIMITER ;

