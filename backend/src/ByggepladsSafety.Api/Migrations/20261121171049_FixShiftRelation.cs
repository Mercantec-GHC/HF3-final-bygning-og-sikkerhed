using System;
using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace ByggepladsSafety.Api.Migrations
{
    /// <inheritdoc />
    public partial class FixShiftRelation : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropTable(
                name: "ShiftWorker");

            migrationBuilder.AddColumn<DateTimeOffset>(
                name: "CheckInTime",
                table: "Shift",
                type: "timestamp with time zone",
                nullable: false,
                defaultValue: new DateTimeOffset(new DateTime(1, 1, 1, 0, 0, 0, 0, DateTimeKind.Unspecified), new TimeSpan(0, 0, 0, 0, 0)));

            migrationBuilder.AddColumn<DateTimeOffset>(
                name: "CheckOutTime",
                table: "Shift",
                type: "timestamp with time zone",
                nullable: true);

            migrationBuilder.AddColumn<bool>(
                name: "IsOnline",
                table: "Shift",
                type: "boolean",
                nullable: false,
                defaultValue: false);

            migrationBuilder.AddColumn<DateTimeOffset>(
                name: "LastHeartbeat",
                table: "Shift",
                type: "timestamp with time zone",
                nullable: true);

            migrationBuilder.AddColumn<int>(
                name: "WorkerId",
                table: "Shift",
                type: "integer",
                nullable: false,
                defaultValue: 0);

            migrationBuilder.CreateIndex(
                name: "IX_Shift_WorkerId",
                table: "Shift",
                column: "WorkerId");

            migrationBuilder.AddForeignKey(
                name: "FK_Shift_Worker_WorkerId",
                table: "Shift",
                column: "WorkerId",
                principalTable: "Worker",
                principalColumn: "Id",
                onDelete: ReferentialAction.Restrict);
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropForeignKey(
                name: "FK_Shift_Worker_WorkerId",
                table: "Shift");

            migrationBuilder.DropIndex(
                name: "IX_Shift_WorkerId",
                table: "Shift");

            migrationBuilder.DropColumn(
                name: "CheckInTime",
                table: "Shift");

            migrationBuilder.DropColumn(
                name: "CheckOutTime",
                table: "Shift");

            migrationBuilder.DropColumn(
                name: "IsOnline",
                table: "Shift");

            migrationBuilder.DropColumn(
                name: "LastHeartbeat",
                table: "Shift");

            migrationBuilder.DropColumn(
                name: "WorkerId",
                table: "Shift");

            migrationBuilder.CreateTable(
                name: "ShiftWorker",
                columns: table => new
                {
                    ShiftsId = table.Column<int>(type: "integer", nullable: false),
                    WorkersId = table.Column<int>(type: "integer", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_ShiftWorker", x => new { x.ShiftsId, x.WorkersId });
                    table.ForeignKey(
                        name: "FK_ShiftWorker_Shift_ShiftsId",
                        column: x => x.ShiftsId,
                        principalTable: "Shift",
                        principalColumn: "Id",
                        onDelete: ReferentialAction.Cascade);
                    table.ForeignKey(
                        name: "FK_ShiftWorker_Worker_WorkersId",
                        column: x => x.WorkersId,
                        principalTable: "Worker",
                        principalColumn: "Id",
                        onDelete: ReferentialAction.Cascade);
                });

            migrationBuilder.CreateIndex(
                name: "IX_ShiftWorker_WorkersId",
                table: "ShiftWorker",
                column: "WorkersId");
        }
    }
}
