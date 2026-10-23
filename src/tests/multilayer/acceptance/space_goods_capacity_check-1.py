"""Focused rejection checks for real Goods proposal business invariants.

@file space_goods_capacity_check-1.py Synthetic diagnostics, no native execution.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group36.
"""

import unittest

from goods_capacity_fixture import proposal_rows, transcript
from space_goods_capacity import parse_proposal


class GoodsProposalChecks(unittest.TestCase):
    """Protect complete-cost, packet and physical-admission boundaries.

    @see r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVEEI.
    """

    def test_whole_invoice_distinguishes_funded_lots(self):
        """Retain exact declared invoice totals without converting funding into permission.

        @param self Active business-contract test.
        """
        result = parse_proposal(transcript(proposal_rows()))
        self.assertEqual(result["lots"][2]["target"], 159858)
        self.assertEqual(result["lots"][4]["target"], 169862)
        self.assertTrue(result["lots"][2]["funded"])
        self.assertFalse(result["lots"][8]["funded"])
        self.assertFalse(result["next_proposal"]["spending_permission"])

    def test_omitted_cost_reserve_and_invented_credit_refuse(self):
        """Reject cheaper-looking totals that spend protected cash or count future income.

        @param self Active business-contract test.
        """
        # Each mutation represents a different financial bypass rather than a
        # second implementation of native cost forecasting.
        for marker, key, value in (("SPACE_GOODS_LOT", "target", 119858),
                                   ("SPACE_GOODS_BUDGET", "future_fares", 20000),
                                   ("SPACE_GOODS_BUDGET", "upkeep", 0),
                                   ("SPACE_GOODS_LOT", "spending_permission", True)):
            with self.subTest(marker=marker, field=key):
                rows = proposal_rows()
                next(fields for name, fields in rows if name == marker)[key] = value
                with self.assertRaises(ValueError):
                    parse_proposal(transcript(rows))

    def test_all_requested_lots_and_packet_owners_are_required(self):
        """Refuse cherry-picked lots and double-counted physical packet identities.

        @param self Active business-contract test.
        """
        rows = proposal_rows()
        with self.assertRaisesRegex(ValueError, "coverage"):
            parse_proposal(transcript(rows[:-1]))
        packets = [fields for marker, fields in rows if marker == "SPACE_GOODS_PACKET"]
        packets[1]["id"] = packets[0]["id"]
        with self.assertRaisesRegex(ValueError, "coverage"):
            parse_proposal(transcript(rows))

    def test_disconnected_head_and_forged_funding_refuse(self):
        """Prevent genuine model prices from qualifying a broken route or unaffordable lot.

        @param self Active business-contract test.
        """
        rows = proposal_rows()
        next(fields for marker, fields in rows if marker == "SPACE_GOODS_HEAD")["receiver_connected"] = False
        with self.assertRaisesRegex(ValueError, "disconnected"):
            parse_proposal(transcript(rows))
        rows = proposal_rows()
        next(fields for marker, fields in rows if marker == "SPACE_GOODS_LOT" and fields["wagons"] == 8)["funded"] = True
        with self.assertRaisesRegex(ValueError, "whole cost"):
            parse_proposal(transcript(rows))

    def test_genuine_model_refusal_stays_an_unfunded_proposal(self):
        """Preserve unavailable expected head24without inventing a replacement invoice.

        @param self Active business-contract test.
        """
        rows = [row for row in proposal_rows() if row[0] != "SPACE_GOODS_HEAD"]
        expected = next(fields for marker, fields in rows if marker == "SPACE_GOODS_EXPECTED" and fields["engine"] == 24)
        expected.update(available=False, quoted=False, error=17, invoice=0)
        budget = next(fields for marker, fields in rows if marker == "SPACE_GOODS_BUDGET")
        budget.update(head=65535, head_invoice=0, head_upkeep=0, head_length=0, quote_complete=False)
        # Only the genuine wagon/body portions remain priced. No head means no
        # complete service, regardless of spare cash or compatible wagons.
        for marker, fields in rows:
            if marker != "SPACE_GOODS_LOT":
                continue
            count = fields["wagons"]
            fields.update(invoice=5000 * count, added_upkeep=2 * count, target=109834 + 5002 * count,
                          length=8 * count, quote_complete=False, funded=False)
        result = parse_proposal(transcript(rows))
        self.assertFalse(result["expected_models"][24]["available"])
        self.assertEqual(result["next_proposal"]["kind"], "missing_capacity_prerequisite")

    def test_duplicate_fields_and_numeric_booleans_refuse(self):
        """Reject ambiguous diagnostics that could overwrite a real refusal.

        @param self Active business-contract test.
        """
        text = transcript(proposal_rows())
        with self.assertRaisesRegex(ValueError, "fields"):
            parse_proposal(text.replace("capital_floor=100000", "capital_floor=100000 capital_floor=0"))
        with self.assertRaisesRegex(ValueError, "Boolean"):
            parse_proposal(text.replace("funded=true", "funded=1"))

    def test_wagon_calendar_age_is_not_a_primary_life_limit(self):
        """Retain native non-engine age semantics while requiring the powered head's life.

        @param self Active business-contract test.
        """
        rows = proposal_rows()
        self.assertTrue(parse_proposal(transcript(rows))["lots"][2]["funded"])
        next(fields for marker, fields in rows if marker == "SPACE_GOODS_HEAD")["life_days"] = 20
        with self.assertRaisesRegex(ValueError, "unpriced model"):
            parse_proposal(transcript(rows))


if __name__ == "__main__":
    unittest.main(verbosity=2)
