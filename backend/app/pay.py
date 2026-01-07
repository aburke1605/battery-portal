import logging

logger = logging.getLogger(__name__)

from flask import Blueprint, jsonify, request
import stripe
import os

from sqlalchemy import update

pay = Blueprint("pay", __name__, url_prefix="/pay")

from app.db import DB

stripe.api_key = os.getenv("STRIPE_SECRET_KEY")


@pay.route("/create-payment-intent", methods=["POST"])
def create_payment_intent():
    data = request.get_json()
    try:
        intent = stripe.PaymentIntent.create(
            amount=int(data["amount"] * 100),
            currency="gbp",
            automatic_payment_methods={"enabled": True},
            metadata={"email": data["email"]},
        )
        return jsonify({"clientSecret": intent.client_secret})
    except stripe.error.StripeError as e:
        return jsonify(error=str(e)), 400


@pay.route("/webhook", methods=["POST"])
def webhook():
    """
    simulate locally with:
      $ stripe listen --forward-to https://localhost:8000/api/pay/webhook --skip-verify
    """
    payload = request.data
    sig_header = request.headers.get("Stripe-Signature")
    endpoint_secret = os.getenv("STRIPE_WEBHOOK_SECRET")

    try:
        event = stripe.Webhook.construct_event(payload, sig_header, endpoint_secret)
    except ValueError:
        return "Invalid payload", 400
    except stripe.error.SignatureVerificationError:
        return "Invalid signature", 400

    if event["type"] == "payment_intent.succeeded":
        intent = event["data"]["object"]

        users_table = DB.Table("users", DB.metadata, autoload_with=DB.engine)
        # fmt: off
        query = (
            update(users_table)
            .where(users_table.c.email == intent["metadata"]["email"])
            .values(subscribed=True)
        )
        # fmt: on
        DB.session.execute(query)
        DB.session.commit()

    elif event["type"] == "payment_intent.payment_failed":
        intent = event["data"]["object"]
        logger.error("Payment failed:", intent["id"])

    return "", 200
