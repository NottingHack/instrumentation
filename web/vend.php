<html>
<meta http-equiv="Content-Type" content="text/html;charset=utf-8" />

  <body>


    <table border="2" cellspacing="2" cellpadding="2">
    <tr>
      <td>Position</td>
      <td>Product</td>
      <td>Description</td>
      <td>Cost</td>
    </tr>

<?php

require "../db.php";

$oInstDB = db_link();

$sSQLVDetails = "
        select 
          coalesce(vr.loc_name, '') as Position, 
          coalesce(p.shortdesc, '') as Product,
          coalesce(p.longdesc, '') as Description,  
          coalesce(concat('£', cast((price/100) as decimal(20,2))), '') as cost
        from vmc_ref vr 
        left outer join vmc_state vs on vr.vmc_ref_id = vs.vmc_ref_id 
        left outer join products p on vs.product_id = p.product_id 
        order by vr.loc_name";

if ($oResult = $oInstDB->query($sSQLVDetails))
{
  while ($oRow = $oResult->fetch_assoc()) 
  {
    echo "    <tr>\n";
    echo "      <td>" . $oRow['Position'] . "</td>\n";
    echo "      <td>" . $oRow['Product'] . "</td>\n";
    echo "      <td>" . $oRow['Description'] . "</td>\n";
    echo "      <td>" . $oRow['cost'] . "</td>\n";
    echo "    </tr>\n";
  }
  
  $oResult->free();
  $oInstDB->close();
} 
  
?>

</table>
</body>
</html>
