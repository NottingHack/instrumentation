<?php

include "dbaccess_header.php";

$sql = <<<SQL
select
  year(l.success_time) as Year,
  monthname(l.success_time) as Month,
  count(*) as "Items vended",
  count(IF(l.vending_machine_id = 1, 1, NULL)) as "Snacks Vended",
  count(IF(l.vending_machine_id = 4, 1, NULL)) as "Drinks Vended",
  concat('£', cast((sum(l.amount_scaled)/100) as decimal(20,2))) as "Vend value",
  concat('£', cast((avg(l.amount_scaled)/100) as decimal(20,2))) as "Avg item cost",
  (
    select concat('£', cast((sum(-1*t.amount)/100) as decimal(20,2)))
    from transactions t
    where year(l.success_time) = year(t.transaction_datetime)
      and month(l.success_time) = month(t.transaction_datetime)
      and t.transaction_status = 'COMPLETE'
      and t.transaction_type = 'TOOL'
  ) as "Laser charges",
  (
    select concat('£', cast((sum(t.amount)/100) as decimal(20,2)))
    from transactions t
    where year(l.success_time) = year(t.transaction_datetime)
      and month(l.success_time) = month(t.transaction_datetime)
      and t.amount > 0
      and t.transaction_status = 'COMPLETE'
  ) as "Payments"
from vend_logs l
where (l.vending_machine_id in (1, 4) or l.vending_machine_id is null)
  and l.success_time is not null
  and (year(l.success_time) > 2012 or (year(l.success_time)=2012 and month(l.success_time) >= 6))
group by year(l.success_time), monthname(l.success_time)
order by year(l.success_time) desc, month(l.success_time) desc;
SQL;

print sql_to_tt_json(db_pdo_link(), $sql);

?>
