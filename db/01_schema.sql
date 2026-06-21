/* warehouse-mfc — schema (SQL Server / LocalDB compatible T-SQL)
   Run:  sqlcmd -S "(localdb)\MSSQLLocalDB" -i db\01_schema.sql            */

IF DB_ID(N'Warehouse') IS NULL
    CREATE DATABASE Warehouse;
GO
USE Warehouse;
GO

IF OBJECT_ID(N'vCurrentStock', N'V')        IS NOT NULL DROP VIEW vCurrentStock;
IF OBJECT_ID(N'sp_RecordMovement', N'P')    IS NOT NULL DROP PROCEDURE sp_RecordMovement;
IF OBJECT_ID(N'StockMovements', N'U')       IS NOT NULL DROP TABLE StockMovements;
IF OBJECT_ID(N'Products', N'U')             IS NOT NULL DROP TABLE Products;
IF OBJECT_ID(N'Warehouses', N'U')           IS NOT NULL DROP TABLE Warehouses;
GO

CREATE TABLE Warehouses (
    WarehouseId INT IDENTITY(1,1) PRIMARY KEY,
    Code        NVARCHAR(16)  NOT NULL UNIQUE,
    Name        NVARCHAR(100) NOT NULL
);

CREATE TABLE Products (
    ProductId    INT IDENTITY(1,1) PRIMARY KEY,
    Sku          NVARCHAR(32)  NOT NULL UNIQUE,
    Name         NVARCHAR(150) NOT NULL,
    Unit         NVARCHAR(16)  NOT NULL CONSTRAINT DF_Products_Unit DEFAULT N'szt',
    ReorderLevel INT           NOT NULL CONSTRAINT DF_Products_Reorder DEFAULT 0
);

CREATE TABLE StockMovements (
    MovementId   BIGINT IDENTITY(1,1) PRIMARY KEY,
    ProductId    INT NOT NULL CONSTRAINT FK_Mov_Product   REFERENCES Products(ProductId),
    WarehouseId  INT NOT NULL CONSTRAINT FK_Mov_Warehouse REFERENCES Warehouses(WarehouseId),
    Qty          INT NOT NULL,                 -- signed: + receive (IN) / - ship (OUT)
    MovementType NVARCHAR(8)   NOT NULL CONSTRAINT CK_Mov_Type CHECK (MovementType IN (N'IN', N'OUT')),
    Note         NVARCHAR(200) NULL,
    CreatedAt    DATETIME2     NOT NULL CONSTRAINT DF_Mov_CreatedAt DEFAULT SYSUTCDATETIME()
);
CREATE INDEX IX_Mov_Product_Wh ON StockMovements(ProductId, WarehouseId);
GO

/* Current on-hand per product x warehouse (a JOIN + GROUP BY), with low-stock flag. */
CREATE VIEW vCurrentStock AS
SELECT  p.ProductId, p.Sku, p.Name AS ProductName, p.Unit, p.ReorderLevel,
        w.WarehouseId, w.Code AS WarehouseCode, w.Name AS WarehouseName,
        COALESCE(SUM(m.Qty), 0) AS OnHand,
        CASE WHEN COALESCE(SUM(m.Qty), 0) <= p.ReorderLevel THEN 1 ELSE 0 END AS IsLow
FROM        Products  p
CROSS JOIN  Warehouses w
LEFT JOIN   StockMovements m
        ON  m.ProductId = p.ProductId AND m.WarehouseId = w.WarehouseId
GROUP BY    p.ProductId, p.Sku, p.Name, p.Unit, p.ReorderLevel,
            w.WarehouseId, w.Code, w.Name;
GO

/* Record a movement inside a transaction; reject an OUT that would go negative.
   @Qty is a positive count; sign is derived from @MovementType. */
CREATE PROCEDURE sp_RecordMovement
    @ProductId    INT,
    @WarehouseId  INT,
    @Qty          INT,
    @MovementType NVARCHAR(8),
    @Note         NVARCHAR(200) = NULL,
    @NewOnHand    INT OUTPUT
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    DECLARE @signed INT = CASE WHEN @MovementType = N'OUT' THEN -ABS(@Qty) ELSE ABS(@Qty) END;

    BEGIN TRAN;
        DECLARE @current INT =
            (SELECT COALESCE(SUM(Qty), 0)
             FROM   StockMovements WITH (UPDLOCK, HOLDLOCK)
             WHERE  ProductId = @ProductId AND WarehouseId = @WarehouseId);

        IF (@current + @signed < 0)
        BEGIN
            ROLLBACK TRAN;
            THROW 50001, N'Brak wystarczajacego stanu dla wydania (OUT).', 1;
        END

        INSERT INTO StockMovements (ProductId, WarehouseId, Qty, MovementType, Note)
        VALUES (@ProductId, @WarehouseId, @signed, @MovementType, @Note);

        SET @NewOnHand = @current + @signed;
    COMMIT TRAN;
END
GO
