/* warehouse-mfc — demo seed data
   Run after 01_schema.sql:
   sqlcmd -S "(localdb)\MSSQLLocalDB" -i db\02_seed.sql                    */

USE Warehouse;
GO

DELETE FROM StockMovements;
DELETE FROM Products;
DELETE FROM Warehouses;
-- Reset IDENTITY so a re-run always produces the *same* ids (Products 1..5, Warehouses 1..2)
-- instead of climbing — otherwise re-seeding invalidates the ids a running app already loaded
-- (FK_Mov_Product violations on the next recorded movement).
DBCC CHECKIDENT('Products',   RESEED, 0);
DBCC CHECKIDENT('Warehouses', RESEED, 0);
DBCC CHECKIDENT('StockMovements', RESEED, 0);
GO

INSERT INTO Warehouses (Code, Name) VALUES
    (N'MAG-A', N'Magazyn główny A'),
    (N'MAG-B', N'Magazyn zapasowy B');

INSERT INTO Products (Sku, Name, Unit, ReorderLevel) VALUES
    (N'4521', N'Młotek stalowy 500g',     N'szt', 10),
    (N'4522', N'Wkrętarka akumulatorowa', N'szt',  5),
    (N'4523', N'Taśma miernicza 5m',      N'szt', 20),
    (N'4524', N'Rękawice robocze L',      N'par', 50),
    (N'4525', N'Śruba M8x40 (100szt)',    N'opak',15);
GO

/* Seed some opening stock via the stored proc so the transaction path is exercised.
   IDs are reset above, so a re-run yields the same 1..N values; we still resolve them by
   natural key (Sku/Code) rather than hardcoding, which is robust either way. */
DECLARE @wA INT = (SELECT WarehouseId FROM Warehouses WHERE Code = N'MAG-A');
DECLARE @wB INT = (SELECT WarehouseId FROM Warehouses WHERE Code = N'MAG-B');
DECLARE @p4521 INT = (SELECT ProductId FROM Products WHERE Sku = N'4521');
DECLARE @p4522 INT = (SELECT ProductId FROM Products WHERE Sku = N'4522');
DECLARE @p4523 INT = (SELECT ProductId FROM Products WHERE Sku = N'4523');
DECLARE @p4524 INT = (SELECT ProductId FROM Products WHERE Sku = N'4524');
DECLARE @p4525 INT = (SELECT ProductId FROM Products WHERE Sku = N'4525');

DECLARE @n INT;
EXEC sp_RecordMovement @ProductId=@p4521, @WarehouseId=@wA, @Qty=40, @MovementType=N'IN', @Note=N'Stan początkowy', @NewOnHand=@n OUTPUT;
EXEC sp_RecordMovement @ProductId=@p4522, @WarehouseId=@wA, @Qty=8,  @MovementType=N'IN', @Note=N'Stan początkowy', @NewOnHand=@n OUTPUT;
EXEC sp_RecordMovement @ProductId=@p4523, @WarehouseId=@wA, @Qty=60, @MovementType=N'IN', @Note=N'Stan początkowy', @NewOnHand=@n OUTPUT;
EXEC sp_RecordMovement @ProductId=@p4524, @WarehouseId=@wA, @Qty=30, @MovementType=N'IN', @Note=N'Stan początkowy', @NewOnHand=@n OUTPUT;  -- below reorder (low)
EXEC sp_RecordMovement @ProductId=@p4525, @WarehouseId=@wB, @Qty=12, @MovementType=N'IN', @Note=N'Stan początkowy', @NewOnHand=@n OUTPUT;  -- below reorder (low)
EXEC sp_RecordMovement @ProductId=@p4521, @WarehouseId=@wA, @Qty=5,  @MovementType=N'OUT',@Note=N'Wydanie na produkcję', @NewOnHand=@n OUTPUT;
GO

SELECT * FROM vCurrentStock ORDER BY WarehouseCode, Sku;
GO
